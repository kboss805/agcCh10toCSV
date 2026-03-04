/**
 * @brief Standalone diagnostic: loads a Ch10 file and dumps TMATS channel info.
 */
#include <cstdio>
#include <cstring>

#include <QCoreApplication>
#include <QDir>
#include <QDebug>

#include "irig106ch10.h"
#include "i106_time.h"
#include "i106_decode_tmats.h"

#include "chapter10reader.h"
#include "channeldata.h"
#include "constants.h"

using namespace Irig106;

static void dumpTmatsFields(const QString& filepath)
{
    printf("=== File: %s ===\n", qPrintable(filepath));

    int file_handle = 0;
    EnI106Status status = enI106Ch10Open(&file_handle,
                                          filepath.toLocal8Bit().constData(),
                                          I106_READ);
    if (status != I106_OK)
    {
        printf("  ERROR: Could not open file (status=%d)\n", status);
        return;
    }

    status = enI106_SyncTime(file_handle, bFALSE, 0);
    if (status != I106_OK)
    {
        printf("  ERROR: SyncTime failed (status=%d)\n", status);
        enI106Ch10Close(file_handle);
        return;
    }

    SuI106Ch10Header header;
    std::vector<unsigned char> buffer(1024 * 1024);

    while (true)
    {
        status = enI106Ch10ReadNextHeader(file_handle, &header);
        if (status != I106_OK)
            break;

        if (buffer.size() < uGetDataLen(&header))
            buffer.resize(uGetDataLen(&header));

        status = enI106Ch10ReadData(file_handle, buffer.size(), buffer.data());
        if (status != I106_OK)
            break;

        if (header.ubyDataType == I106CH10_DTYPE_TMATS)
        {
            SuTmatsInfo tmats_info;
            memset(&tmats_info, 0, sizeof(tmats_info));
            status = enI106_Decode_Tmats(&header, buffer.data(), &tmats_info);
            if (status != I106_OK)
            {
                printf("  ERROR: Decode_Tmats failed (status=%d)\n", status);
                break;
            }

            printf("\n  TMATS R-Record Data Sources:\n");
            SuRRecord* record = tmats_info.psuFirstRRecord;
            int r_idx = 0;
            while (record != nullptr)
            {
                printf("  R-Record #%d:\n", r_idx);
                SuRDataSource* ds = record->psuFirstDataSource;
                int ds_idx = 0;
                while (ds != nullptr)
                {
                    printf("    DataSource #%d:\n", ds_idx);
                    printf("      szTrackNumber:       %s\n",
                           ds->szTrackNumber ? ds->szTrackNumber : "(null)");
                    printf("      szDataSourceID:      %s\n",
                           ds->szDataSourceID ? ds->szDataSourceID : "(null)");
                    printf("      szChannelDataType:   %s\n",
                           ds->szChannelDataType ? ds->szChannelDataType : "(null)");
                    printf("      szChanDataLinkName:  %s\n",
                           ds->szChanDataLinkName ? ds->szChanDataLinkName : "(null)");
                    printf("      szPcmDataLinkName:   %s\n",
                           ds->szPcmDataLinkName ? ds->szPcmDataLinkName : "(null)");
                    ds = ds->psuNext;
                    ds_idx++;
                }
                record = record->psuNext;
                r_idx++;
            }
            break; // Only need first TMATS packet
        }
    }

    // Now also load via Chapter10Reader and dump what it produces
    printf("\n  Chapter10Reader combo box lists:\n");
    Chapter10Reader reader;
    reader.loadChannels(filepath);

    QStringList time_list = reader.getTimeChannelComboBoxList();
    printf("    Time channels (%d):\n", time_list.size());
    for (const QString& entry : time_list)
        printf("      \"%s\"\n", qPrintable(entry));

    QStringList pcm_list = reader.getPCMChannelComboBoxList();
    printf("    PCM channels (%d):\n", pcm_list.size());
    for (const QString& entry : pcm_list)
        printf("      \"%s\"\n", qPrintable(entry));

    enI106Ch10Close(file_handle);
    printf("\n");
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QDir dataDir(QCoreApplication::applicationDirPath());
    dataDir.cdUp();
    dataDir.cd("data");

    QStringList ch10Files = dataDir.entryList({"*.ch10"}, QDir::Files);
    if (ch10Files.isEmpty())
    {
        printf("No .ch10 files found in %s\n", qPrintable(dataDir.absolutePath()));
        return 1;
    }

    for (const QString& file : ch10Files)
    {
        dumpTmatsFields(dataDir.filePath(file));
    }

    return 0;
}
