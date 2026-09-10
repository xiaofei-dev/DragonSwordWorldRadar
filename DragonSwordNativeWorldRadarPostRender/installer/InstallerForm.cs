using System;
using System.Drawing;
using System.Windows.Forms;

namespace DragonSwordNativeWorldRadarPostRender.Installer
{
    internal sealed class InstallerForm : Form
    {
        private readonly TextBox _gamePath = new TextBox();
        private readonly TextBox _status = new TextBox();
        private readonly ComboBox _settingsKey = new ComboBox();
        private readonly ComboBox _enableKey = new ComboBox();
        private readonly ComboBox _disableKey = new ComboBox();
        private readonly Button _browse = new Button();
        private readonly Button _uninstall = new Button();
        private readonly Button _install = new Button();
        private readonly Button _close = new Button();
        private InstallerInstallationState _installationState;
        private bool _busy;
        private string _loadedGamePath;

        internal InstallerForm()
        {
            Text = "DragonSword Native World Radar 3.0.0 Setup";
            ClientSize = new Size(760, 510);
            MinimumSize = new Size(776, 549);
            StartPosition = FormStartPosition.CenterScreen;
            Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point);
            MaximizeBox = false;

            Controls.Add(new Label
            {
                AutoSize = true,
                Font = new Font(Font.FontFamily, 16F, FontStyle.Bold),
                Location = new Point(24, 18),
                Text = "DragonSword Native World Radar"
            });

            Controls.Add(new Label
            {
                AutoSize = false,
                Location = new Point(27, 58),
                Size = new Size(706, 62),
                Text = "Select DSClient-Win64-Shipping.exe. Setup accepts a structurally complete " +
                       "ExperimentalNested UE4SS layout without a loader hash allowlist. Missing, root, " +
                       "dual, or incomplete layouts use the backed-up migration path."
            });

            Controls.Add(new Label
            {
                AutoSize = true,
                Location = new Point(27, 128),
                Text = "Game executable"
            });

            _gamePath.Location = new Point(30, 150);
            _gamePath.Size = new Size(600, 23);
            _gamePath.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            _gamePath.Leave += delegate { RefreshInstallationState(false); };
            Controls.Add(_gamePath);

            _browse.Location = new Point(640, 148);
            _browse.Size = new Size(92, 27);
            _browse.Text = "Browse...";
            _browse.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            _browse.Click += BrowseClick;
            Controls.Add(_browse);

            AddHotkeyControl(_settingsKey, "Settings key", "F6", 30);
            AddHotkeyControl(_enableKey, "Enable key", "F7", 268);
            AddHotkeyControl(_disableKey, "Disable key", "F8", 506);
            Controls.Add(new Label
            {
                Location = new Point(30, 249),
                Size = new Size(702, 46),
                Text = "Choose three different keys (for example INSERT / HOME / PAGEUP). " +
                       "Install, Update and Repair apply these keys after confirmation. " +
                       "Avoid game or other Mod bindings. Restart the game to apply."
            });

            _status.Location = new Point(30, 310);
            _status.Size = new Size(702, 128);
            _status.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            _status.Multiline = true;
            _status.ReadOnly = true;
            _status.ScrollBars = ScrollBars.Vertical;
            _status.BackColor = SystemColors.Window;
            _status.Text = "Ready. Public diagnostics default to Off. This installer is unsigned.";
            Controls.Add(_status);

            _close.Location = new Point(493, 460);
            _close.Size = new Size(96, 30);
            _close.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            _close.Text = "Close";
            _close.Click += delegate { Close(); };
            Controls.Add(_close);

            _uninstall.Location = new Point(381, 460);
            _uninstall.Size = new Size(102, 30);
            _uninstall.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            _uninstall.Text = "Uninstall";
            _uninstall.ForeColor = Color.DarkRed;
            _uninstall.Enabled = false;
            _uninstall.Click += UninstallClick;
            Controls.Add(_uninstall);

            _install.Location = new Point(603, 460);
            _install.Size = new Size(129, 30);
            _install.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            _install.Text = "Install";
            _install.Font = new Font(Font, FontStyle.Bold);
            _install.Enabled = false;
            _install.Click += InstallClick;
            Controls.Add(_install);

            AcceptButton = _install;
            CancelButton = _close;

            var discovered = InstallerEngine.TryDiscoverGameExecutable();
            if (!string.IsNullOrEmpty(discovered))
            {
                _gamePath.Text = discovered;
                RefreshInstallationState(true);
            }
            else
            {
                UpdateActionButtons();
            }
        }

        private void AddHotkeyControl(ComboBox box, string label, string defaultKey, int left)
        {
            Controls.Add(new Label { AutoSize = true, Location = new Point(left, 194), Text = label });
            box.Location = new Point(left, 216);
            box.Size = new Size(226, 23);
            box.DropDownStyle = ComboBoxStyle.DropDown;
            box.MaxLength = 32;
            box.MaxDropDownItems = 12;
            box.AccessibleName = label;
            for (int key = 1; key <= 24; ++key) box.Items.Add("F" + key);
            foreach (var name in new[] { "INSERT", "HOME", "PAGEUP", "PAGEDOWN", "END", "DELETE", "SPACE" }) box.Items.Add(name);
            for (char key = 'A'; key <= 'Z'; ++key) box.Items.Add(key.ToString());
            for (int key = 0; key <= 9; ++key) { box.Items.Add(key.ToString()); box.Items.Add("NUM" + key); }
            box.Text = defaultKey;
            Controls.Add(box);
        }

        private void BrowseClick(object sender, EventArgs e)
        {
            using (var dialog = new OpenFileDialog())
            {
                dialog.Title = "Select DSClient-Win64-Shipping.exe";
                dialog.Filter = "DragonSword executable (DSClient-Win64-Shipping.exe)|DSClient-Win64-Shipping.exe";
                dialog.CheckFileExists = true;
                dialog.Multiselect = false;
                if (dialog.ShowDialog(this) == DialogResult.OK)
                {
                    _gamePath.Text = dialog.FileName;
                    RefreshInstallationState(true);
                }
            }
        }

        private void RefreshInstallationState(bool reloadHotkeys)
        {
            if (_busy)
            {
                return;
            }
            try
            {
                _installationState = InstallerEngine.InspectInstallationState(_gamePath.Text);
                if (reloadHotkeys || !string.Equals(_loadedGamePath, _gamePath.Text.Trim(), StringComparison.OrdinalIgnoreCase))
                {
                    var hotkeys = _installationState.Hotkeys;
                    _settingsKey.Text = hotkeys.Settings;
                    _enableKey.Text = hotkeys.Enable;
                    _disableKey.Text = hotkeys.Disable;
                    _loadedGamePath = _gamePath.Text.Trim();
                }
                _status.Text = _installationState.StatusDescription;
            }
            catch (Exception exception)
            {
                _installationState = null;
                _status.Text = "Installation state could not be inspected." + Environment.NewLine +
                    exception.Message;
            }
            UpdateActionButtons();
        }

        private void UpdateActionButtons()
        {
            var canInstall = _installationState != null &&
                (_installationState.CanInstall || _installationState.CanUpdate);
            _install.Text = _installationState != null && _installationState.CanUpdate
                ? string.Equals(_installationState.InstalledVersion, "3.0.0", StringComparison.Ordinal)
                    ? "Repair"
                    : "Update"
                : "Install";
            _install.Enabled = !_busy && canInstall;
            _uninstall.Enabled = !_busy && _installationState != null &&
                _installationState.CanUninstall;
            _browse.Enabled = !_busy;
            _gamePath.Enabled = !_busy;
            _settingsKey.Enabled = !_busy && canInstall;
            _enableKey.Enabled = !_busy && canInstall;
            _disableKey.Enabled = !_busy && canInstall;
            _close.Enabled = !_busy;
        }

        private void InstallClick(object sender, EventArgs e)
        {
            RefreshInstallationState(false);
            if (_installationState == null ||
                (!_installationState.CanInstall && !_installationState.CanUpdate))
            {
                MessageBox.Show(
                    this,
                    _status.Text,
                    "Install or update unavailable",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }
            var requestedUpdate = _installationState.CanUpdate;
            var completed = false;
            SetBusy(true);
            try
            {
                _status.Text = "Inspecting the game, UE4SS structure, paths, existing Radar, and embedded release identity...";
                _status.Refresh();
                var plan = InstallerEngine.InspectWithHotkeys(_gamePath.Text,
                    _settingsKey.Text, _enableKey.Text, _disableKey.Text);
                _status.Text =
                    (requestedUpdate ? "Update / Repair plan completed." : "Installation plan completed.") + Environment.NewLine +
                    "Detected: " + plan.LayoutDescription + Environment.NewLine +
                    "Payload: " + plan.PluginDescription + Environment.NewLine +
                    "Action: " + plan.ActionDescription + Environment.NewLine + plan.Hotkeys.Description;
                var confirmation = MessageBox.Show(
                    this,
                    "Detected UE4SS: " + plan.LayoutDescription + Environment.NewLine +
                    "Radar payload: " + plan.PluginDescription + Environment.NewLine + Environment.NewLine +
                    plan.ActionDescription + Environment.NewLine + Environment.NewLine +
                    "UE4SS directory: " + plan.UE4SSDirectory + Environment.NewLine +
                    "Radar directory: " + plan.ModDirectory + Environment.NewLine + Environment.NewLine +
                    "Keys to apply: " + plan.Hotkeys.Description + Environment.NewLine + Environment.NewLine +
                    (plan.ConvertsUE4SS
                        ? "WARNING: The detected UE4SS loader is not compatible with this Radar build. " +
                          "Continuing will back up every loader file that is replaced or deactivated, install the pinned " +
                          "ExperimentalNested loader, and migrate existing Mods, Mod configuration, and mods.txt state into " +
                          "the Experimental layout. A complete original-layout UE4SS backup is verified first; the old active " +
                          "loader, Mods, settings, and configuration are then removed from their original locations and remain " +
                          "only in that backup. Third-party native DLL Mods may still need Experimental-compatible builds."
                        : "The existing structurally complete UE4SS loader and proxy, unrelated Mods, settings, and mods.txt content will be preserved. " +
                          "Visibility, diagnostics, and treasure-ignore settings are preserved. Setup uses a temporary rollback journal and removes it after success.") + Environment.NewLine + Environment.NewLine +
                    "Continue?",
                    plan.ConvertsUE4SS
                        ? "Confirm UE4SS conversion"
                        : requestedUpdate
                            ? "Confirm Radar update / repair"
                            : "Confirm Radar installation",
                    MessageBoxButtons.YesNo,
                    plan.ConvertsUE4SS ? MessageBoxIcon.Warning : MessageBoxIcon.Information,
                    MessageBoxDefaultButton.Button2);
                if (confirmation != DialogResult.Yes)
                {
                    _status.Text = "Installation cancelled. No files were changed.";
                    return;
                }

                _status.Text = "Revalidating the confirmed plan and installing transactionally...";
                _status.Refresh();
                var result = InstallerEngine.InstallConfirmedWithHotkeys(
                    _gamePath.Text,
                    plan.Hotkeys.Settings, plan.Hotkeys.Enable, plan.Hotkeys.Disable,
                    plan.IdentityToken);
                completed = true;
                _status.Text =
                    (requestedUpdate || result.UpdatedExistingRadar
                        ? "Update / Repair completed successfully."
                        : "Installation completed successfully.") + Environment.NewLine +
                    "UE4SS layout: " + result.LayoutDescription + Environment.NewLine +
                    "Mod directory: " + result.ModDirectory + Environment.NewLine +
                    "Controlling mods.txt: " + result.ModsTxtPath + Environment.NewLine +
                    result.Hotkeys.Description + Environment.NewLine +
                    (string.IsNullOrEmpty(result.BackupDirectory)
                        ? "Persistent backup: not required"
                        : "Conversion backup and install log: " + result.BackupDirectory);
                MessageBox.Show(
                    this,
                    (requestedUpdate || result.UpdatedExistingRadar ? "Update / Repair successful. " : "Installation successful. ") +
                        result.Hotkeys.Description + ". Restart the game, load the open world, and press " +
                        result.Hotkeys.Enable + " to enable Radar. Run Setup again to change these keys through Update / Repair.",
                    requestedUpdate || result.UpdatedExistingRadar ? "Update / Repair successful" : "Installation successful",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
            }
            catch (Exception exception)
            {
                _status.Text = "Installation stopped safely." + Environment.NewLine + exception.Message;
                MessageBox.Show(
                    this,
                    exception.Message,
                    "Installation stopped",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            finally
            {
                SetBusy(false);
                RefreshInstallationState(completed);
            }
        }

        private void UninstallClick(object sender, EventArgs e)
        {
            RefreshInstallationState(false);
            if (_installationState == null || !_installationState.CanUninstall)
            {
                MessageBox.Show(
                    this,
                    _status.Text,
                    "Uninstall unavailable",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }
            SetBusy(true);
            try
            {
                _status.Text =
                    "Inspecting the selected game path and verifying strict Native World Radar ownership...";
                _status.Refresh();
                var plan = InstallerEngine.InspectUninstall(_gamePath.Text);
                _status.Text =
                    "Uninstall plan completed." + Environment.NewLine +
                    "Detected: " + plan.LayoutDescription + Environment.NewLine +
                    "Radar directory: " + plan.ModDirectory + Environment.NewLine +
                    "Controlling mods.txt: " + plan.ModsTxtPath;
                var confirmation = MessageBox.Show(
                    this,
                    "Remove DragonSword Native World Radar?" + Environment.NewLine + Environment.NewLine +
                    "Radar directory: " + plan.ModDirectory + Environment.NewLine +
                    "Controlling mods.txt: " + plan.ModsTxtPath + Environment.NewLine + Environment.NewLine +
                    "This removes the strictly owned Radar directory, including its settings, logs, and caches, " +
                    "and removes only its exact mods.txt entry. UE4SS, unrelated Mods, and game saves are preserved." +
                    Environment.NewLine + Environment.NewLine +
                    "Continue?",
                    "Confirm Native World Radar uninstall",
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Warning,
                    MessageBoxDefaultButton.Button2);
                if (confirmation != DialogResult.Yes)
                {
                    _status.Text = "Uninstall cancelled. No files were changed.";
                    return;
                }

                _status.Text =
                    "Revalidating the confirmed uninstall plan and removing Radar transactionally...";
                _status.Refresh();
                var result = InstallerEngine.UninstallConfirmed(
                    _gamePath.Text,
                    plan.IdentityToken);
                _status.Text =
                    "Uninstall completed successfully." + Environment.NewLine +
                    "Removed Mod directory: " + result.ModDirectory + Environment.NewLine +
                    "Updated mods.txt: " + result.ModsTxtPath + Environment.NewLine +
                    "UE4SS, unrelated Mods, and game saves were preserved.";
                MessageBox.Show(
                    this,
                    "Native World Radar was removed successfully. UE4SS, unrelated Mods, and game saves were preserved.",
                    "Uninstall successful",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
            }
            catch (Exception exception)
            {
                _status.Text =
                    "Uninstall stopped safely." + Environment.NewLine +
                    exception.Message;
                MessageBox.Show(
                    this,
                    exception.Message,
                    "Uninstall stopped",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            finally
            {
                SetBusy(false);
                RefreshInstallationState(true);
            }
        }

        private void SetBusy(bool busy)
        {
            _busy = busy;
            UseWaitCursor = busy;
            UpdateActionButtons();
        }
    }
}
