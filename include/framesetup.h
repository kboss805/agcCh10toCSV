/**
 * @file framesetup.h
 * @brief PCM frame parameter definitions and INI-based frame setup loading.
 */

#ifndef FRAMESETUP_H
#define FRAMESETUP_H

#include <QObject>
#include <QSettings>

/**
 * @brief Describes one named parameter within a PCM minor frame.
 *
 * Each parameter maps to a word position in the frame and carries calibration
 * values (slope/scale) used to convert raw 16-bit samples to engineering units.
 */
struct ParameterInfo
{
    QString name;      ///< Parameter name (e.g., "L_RCVR1").
    int word;          ///< Zero-based word index within the minor frame.
    double slope;      ///< Calibration slope (dB per raw count).
    double scale;      ///< Calibration offset applied before slope.
    bool is_enabled;   ///< Whether this parameter is included in output.
    double sample_sum; ///< Running sum of scaled values for averaging.
};

/**
 * @brief Loads and manages the list of PCM frame parameters.
 *
 * Parameters are read from an INI file that maps receiver/channel names
 * to word positions within the PCM minor frame. The class also supports
 * saving the current parameter configuration back to a QSettings file.
 */
class FrameSetup : public QObject
{
    Q_OBJECT

public:
    FrameSetup(QObject* parent = nullptr);

    /**
     * @brief Loads parameters from an INI file.
     * @param[in] filename Path to the INI file.
     * @param[in] num_words_in_minor_frame Number of words per minor frame.
     * @return true if at least one parameter was loaded.
     */
    bool tryLoadingFile(const QString& filename, int num_words_in_minor_frame);

    /// Saves the current parameter list to @p settings.
    void saveToSettings(QSettings& settings);

    int length() const; ///< @return Number of parameters.

    /// @return Mutable pointer to the parameter at index @p i.
    ParameterInfo* getParameter(int i);
    /// @return Const pointer to the parameter at index @p i.
    const ParameterInfo* getParameter(int i) const;

    /// Removes all parameters.
    void clearParameters();

private:
    QList<ParameterInfo> m_parameters; ///< Ordered list of frame parameters.
};

#endif // FRAMESETUP_H
