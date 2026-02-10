#include "chapter10reader.h"

#include <QDebug>

using namespace Irig106;

Chapter10Reader::Chapter10Reader(QObject* parent) :
    QObject(parent)
{
    // set on UTC
    putenv("TZ=GMT0");
    tzset();

    m_buffer = nullptr;
    m_buffer_size = 0L;

    m_current_time_channel = -1; // no channel selected
    m_current_pcm_channel = -1;

    m_file_start_time = nullptr;
    m_file_stop_time = nullptr;

}

Chapter10Reader::~Chapter10Reader()
{
    if (m_buffer)
        free(m_buffer);
    if (m_file_start_time)
        free(m_file_start_time);
    if (m_file_stop_time)
        free(m_file_stop_time);
    qDeleteAll(m_channel_data);
}

bool Chapter10Reader::tryLoadingFile(const QString& filename)
{
    m_status = enI106Ch10Open(&m_file_handle, filename.toUtf8().constData(), I106_READ);

    if (m_status != I106_OK && m_status != I106_OPEN_WARNING)
    {
        emit displayErrorMessage(QString("Error opening data file."));
        return false;
    }

    m_status = enI106_SyncTime(m_file_handle, bFALSE, 0);

    if (m_status != I106_OK)
    {
        emit displayErrorMessage(QString("Error establishing time sync."));
        return false;
    }

    m_filename = filename;

    return true;
}

void Chapter10Reader::closeFile()
{
    enI106Ch10Close(m_file_handle);
    if (m_buffer)
        free(m_buffer);
    m_buffer = nullptr;
    m_buffer_size = 0L;
}

void Chapter10Reader::clearSettings()
{
    qDeleteAll(m_channel_data);
    m_channel_data.clear();
    m_time_channels.clear();
    m_pcm_channels.clear();
    m_current_time_channel = -1; // no channel selected
    m_current_pcm_channel = -1;
}

bool Chapter10Reader::loadChannels(const QString& filename)
{
    if (!tryLoadingFile(filename))
        return false;

    bool found_start_time = false;

    while (true)
    {
        m_status = enI106Ch10ReadNextHeader(m_file_handle, &m_header);

        // Check for end of file or read errors
        if (m_status != I106_OK)
            break;

        // Make sure our buffer is big enough
        if (m_buffer_size < uGetDataLen(&m_header))
        {
            unsigned char* new_buffer = (unsigned char*)realloc(m_buffer, uGetDataLen(&m_header));
            if (!new_buffer)
            {
                emit displayErrorMessage("Memory allocation failed.");
                closeFile();
                return false;
            }
            m_buffer = new_buffer;
            m_buffer_size = uGetDataLen(&m_header);

            qDebug() << "resized buffer size to " << m_buffer_size;
        }

        // Read the data buffer
        m_status = enI106Ch10ReadData(m_file_handle, m_buffer_size, m_buffer);

        // Check for data read errors
        if (m_status != I106_OK)
            break;

        // Update the count for channel
        addChannelInfoEntry(m_header.uChID);
        m_channel_data[m_header.uChID]->incrementChannelCount();

        // Save data start and stop times
        if ((m_header.ubyDataType != I106CH10_DTYPE_TMATS) &&
            (m_header.ubyDataType != I106CH10_DTYPE_IRIG_TIME) &&
            (m_header.ubyDataType != I106CH10_DTYPE_RECORDING_INDEX))
        {
            if (!found_start_time)
            {
                memcpy((char *)m_relative_start_time, (char *)m_header.aubyRefTime, 6);
                found_start_time = true;
            }
            else
            {
                memcpy((char *)m_relative_stop_time, (char *)m_header.aubyRefTime, 6);
            }
        }

        // If the header indicates a TMATS packet, parse it for channel types/names
        if (m_header.ubyDataType == I106CH10_DTYPE_TMATS)
        {
            memset(&m_tmats_info, 0, sizeof(m_tmats_info));
            m_status = enI106_Decode_Tmats(&m_header, m_buffer, &m_tmats_info);
            if (m_status != I106_OK)
                break;

            // Find channels mentioned in TMATS record
            SuRRecord* r_record = m_tmats_info.psuFirstRRecord;
            SuRDataSource* r_data_source;
            int track_number;
            while (r_record != nullptr)
            {
                // Get the first data source for this R record
                r_data_source = r_record->psuFirstDataSource;
                while (r_data_source != nullptr)
                {
                    // Make sure an entry exists
                    track_number = atoi(r_data_source->szTrackNumber);
                    addChannelInfoEntry(track_number);

                    // Save channel type and name
                    m_channel_data[track_number]->setChannelType(QString(r_data_source->szChannelDataType));
                    m_channel_data[track_number]->setChannelName(QString(r_data_source->szDataSourceID));

                    // Get the next R record data source
                    r_data_source = r_data_source->psuNext;
                }
                // Get the next R record
                r_record = r_record->psuNext;

            } // end while walking R record linked list
        }
    }

    // Translate start and stop times
    SuIrig106Time start_real_time;
    if (!m_file_start_time)
    {
        m_file_start_time = (tm*)malloc(sizeof(tm));
        if (!m_file_start_time)
        {
            emit displayErrorMessage("Memory allocation failed.");
            closeFile();
            return false;
        }
    }
    enI106_Rel2IrigTime(m_file_handle, m_relative_start_time, &start_real_time);
    gmtime_s(m_file_start_time, (time_t *)&(start_real_time.ulSecs));

    SuIrig106Time stop_real_time;
    if (!m_file_stop_time)
    {
        m_file_stop_time = (tm*)malloc(sizeof(tm));
        if (!m_file_stop_time)
        {
            emit displayErrorMessage("Memory allocation failed.");
            closeFile();
            return false;
        }
    }
    enI106_Rel2IrigTime(m_file_handle, m_relative_stop_time, &stop_real_time);
    gmtime_s(m_file_stop_time, (time_t *)&(stop_real_time.ulSecs));

    m_time_difference = start_real_time.ulSecs - (m_file_start_time->tm_yday*24*60*60 +
                                                  m_file_start_time->tm_hour*60*60 +
                                                  m_file_start_time->tm_min*60 +
                                                  m_file_start_time->tm_sec);

    // Go through m_channel_info and get the time channels and PCM channels
    for (auto it = m_channel_data.begin(); it != m_channel_data.end(); it++)
    {
        ChannelData* channel_info = it.value();

        // Ignore channels with no data
        if (channel_info->channelCount() == 0)
            continue;

        // Store time channels and PCM channels
        if (channel_info->channelType() == PCMConstants::kChannelTypeTime)
            m_time_channels.append(channel_info);
        else if (channel_info->channelType() == PCMConstants::kChannelTypePcm)
            m_pcm_channels.append(channel_info);
    }

    closeFile();
    return true;
}

void Chapter10Reader::addChannelInfoEntry(int channel_id)
{
    // If an entry for channel_id doesn't exist in m_channel_info, make one.
    // If the entry already exists, do nothing.
    if (!m_channel_data.contains(channel_id))
        m_channel_data.insert(channel_id, new ChannelData(channel_id));
}

QStringList Chapter10Reader::buildChannelComboBoxList(const QList<ChannelData*>& channels) const
{
    QStringList list;
    for (const auto* channel : channels)
        list.append(QString::number(channel->channelID()) + " - " + channel->channelName());
    return list;
}

QStringList Chapter10Reader::getTimeChannelComboBoxList() const
{
    return buildChannelComboBoxList(m_time_channels);
}

QStringList Chapter10Reader::getPCMChannelComboBoxList() const
{
    return buildChannelComboBoxList(m_pcm_channels);
}

// DOY is 1-indexed, so add 1
int Chapter10Reader::getStartDayOfYear() const
{
    return m_file_start_time ? m_file_start_time->tm_yday + 1 : 0;
}

int Chapter10Reader::getStartHour() const
{
    return m_file_start_time ? m_file_start_time->tm_hour : 0;
}

int Chapter10Reader::getStartMinute() const
{
    return m_file_start_time ? m_file_start_time->tm_min : 0;
}

int Chapter10Reader::getStartSecond() const
{
    return m_file_start_time ? m_file_start_time->tm_sec : 0;
}

// DOY is 1-indexed, so add 1
int Chapter10Reader::getStopDayOfYear() const
{
    return m_file_stop_time ? m_file_stop_time->tm_yday + 1 : 0;
}

int Chapter10Reader::getStopHour() const
{
    return m_file_stop_time ? m_file_stop_time->tm_hour : 0;
}

int Chapter10Reader::getStopMinute() const
{
    return m_file_stop_time ? m_file_stop_time->tm_min : 0;
}

int Chapter10Reader::getStopSecond() const
{
    return m_file_stop_time ? m_file_stop_time->tm_sec : 0;
}

void Chapter10Reader::timeChannelChanged(int combobox_index)
{
    // subtract 1 from the index to account for "Select a Time Stream"
    int list_index = combobox_index - 1;
    if (list_index < 0)
        m_current_time_channel = -1;
    else
        m_current_time_channel = m_time_channels[list_index]->channelID();
}

void Chapter10Reader::pcmChannelChanged(int combobox_index)
{
    // subtract 1 from the index to account for "Select a PCM Stream"
    int list_index = combobox_index - 1;
    if (list_index < 0)
        m_current_pcm_channel = -1;
    else
        m_current_pcm_channel = m_pcm_channels[list_index]->channelID();
}

uint64_t Chapter10Reader::dhmsToUInt64(int day, int hour, int minute, int second) const
{
    return m_time_difference +
            (day - 1) * 24 * 60 * 60 +
            hour * 60 * 60 +
            minute * 60 +
            second;
}

int Chapter10Reader::getTimeChannelIndex(int channel_id) const
{
    for (int i = 0; i < m_time_channels.size(); i++)
    {
        if (m_time_channels[i]->channelID() == channel_id)
            return i;
    }
    return -1;
}

int Chapter10Reader::getPCMChannelIndex(int channel_id) const
{
    for (int i = 0; i < m_pcm_channels.size(); i++)
    {
        if (m_pcm_channels[i]->channelID() == channel_id)
            return i;
    }
    return -1;
}

int Chapter10Reader::getCurrentTimeChannelID() const
{
    return m_current_time_channel;
}

int Chapter10Reader::getCurrentPCMChannelID() const
{
    return m_current_pcm_channel;
}
