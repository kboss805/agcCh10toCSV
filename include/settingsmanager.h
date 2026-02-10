/**
 * @file settingsmanager.h
 * @brief Persists and restores application state via INI files.
 */

#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>

class MainViewModel;

/**
 * @brief Reads and writes application settings to INI configuration files.
 *
 * Acts as a mediator between the MainViewModel state and on-disk INI files.
 * Uses SettingsData as the transfer format.
 */
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    /// @param[in] view_model Pointer to the owning MainViewModel.
    SettingsManager(MainViewModel* view_model);

    /// Loads settings from @p filename and applies them to the MainViewModel.
    void loadFile(const QString& filename);
    /// Captures current MainViewModel state and saves it to @p filename.
    void saveFile(const QString& filename);

private:
    MainViewModel* m_view_model; ///< Owning MainViewModel instance.
};

#endif // SETTINGSMANAGER_H
