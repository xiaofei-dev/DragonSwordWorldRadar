using System;
using System.Drawing;
using System.IO;
using System.Windows.Forms;

namespace DragonSwordNativeAutoPickup.Installer
{
    internal sealed class InstallerForm : Form
    {
        private readonly TextBox _gamePath = new TextBox();
        private readonly TextBox _hotkey = new TextBox();
        private readonly TextBox _interactionKeyFallback = new TextBox();
        private readonly ComboBox _rangeSelection = new ComboBox();
        private readonly TextBox _status = new TextBox();
        private readonly Button _browse = new Button();
        private readonly Button _install = new Button();
        private readonly Button _uninstall = new Button();
        private readonly Button _close = new Button();
        private AutoPickupInstallationState110 _installationState;
        private bool _busy;

        internal InstallerForm()
        {
            Text = "DragonSword Native Auto Pickup 1.3.1 Setup";
            ClientSize = new Size(720, 545);
            MinimumSize = new Size(736, 584);
            StartPosition = FormStartPosition.CenterScreen;
            Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point);
            MaximizeBox = false;

            var title = new Label
            {
                AutoSize = true,
                Font = new Font(Font.FontFamily, 16F, FontStyle.Bold),
                Location = new Point(22, 18),
                Text = "DragonSword Native Auto Pickup"
            };
            Controls.Add(title);

            var introduction = new Label
            {
                AutoSize = false,
                Location = new Point(25, 55),
                Size = new Size(670, 42),
                Text = "Choose DSClient-Win64-Shipping.exe. Setup installs, upgrades, or repairs Auto Pickup, " +
                       "can safely remove an owned installation, and preserves unrelated Mods."
            };
            Controls.Add(introduction);

            var gameLabel = new Label
            {
                AutoSize = true,
                Location = new Point(25, 105),
                Text = "Game executable"
            };
            Controls.Add(gameLabel);

            _gamePath.Location = new Point(28, 126);
            _gamePath.Size = new Size(568, 23);
            _gamePath.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            _gamePath.Leave += delegate { RefreshInstallationState(false); };
            Controls.Add(_gamePath);

            _browse.Location = new Point(606, 124);
            _browse.Size = new Size(88, 27);
            _browse.Text = "Browse...";
            _browse.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            _browse.Click += BrowseClick;
            Controls.Add(_browse);

            var hotkeyLabel = new Label
            {
                AutoSize = true,
                Location = new Point(25, 169),
                Text = "Toggle key (Auto Pickup starts off every game)"
            };
            Controls.Add(hotkeyLabel);

            _hotkey.Location = new Point(28, 190);
            _hotkey.Size = new Size(112, 23);
            _hotkey.CharacterCasing = CharacterCasing.Upper;
            _hotkey.Text = "F9";
            Controls.Add(_hotkey);

            var hotkeyHelp = new Label
            {
                AutoSize = true,
                Location = new Point(151, 194),
                Text = "Examples: F9, F10, K, NUM3, HOME"
            };
            Controls.Add(hotkeyHelp);

            var interactionFallbackLabel = new Label
            {
                AutoSize = true,
                Location = new Point(25, 229),
                Text = "Fallback interaction key (used only if AUTO detection fails)"
            };
            Controls.Add(interactionFallbackLabel);

            _interactionKeyFallback.Location = new Point(28, 250);
            _interactionKeyFallback.Size = new Size(190, 23);
            _interactionKeyFallback.Text = "F";
            Controls.Add(_interactionKeyFallback);

            var interactionFallbackHelp = new Label
            {
                AutoSize = true,
                Location = new Point(229, 254),
                Text = "Examples: F, E, K, Gamepad_FaceButton_Bottom"
            };
            Controls.Add(interactionFallbackHelp);

            var rangeLabel = new Label
            {
                AutoSize = true,
                Location = new Point(25, 289),
                Text = "Optional native interaction range"
            };
            Controls.Add(rangeLabel);

            _rangeSelection.DropDownStyle = ComboBoxStyle.DropDownList;
            _rangeSelection.Location = new Point(28, 311);
            _rangeSelection.Size = new Size(180, 23);
            _rangeSelection.Items.Add(new RangeOption("None (original range)", InstallerEngine.RangeSelection.None));
            _rangeSelection.Items.Add(new RangeOption("3x range", InstallerEngine.RangeSelection.X3));
            _rangeSelection.Items.Add(new RangeOption("5x range", InstallerEngine.RangeSelection.X5));
            _rangeSelection.Items.Add(new RangeOption("10x range", InstallerEngine.RangeSelection.X10));
            _rangeSelection.Items.Add(new RangeOption("15x range", InstallerEngine.RangeSelection.X15));
            _rangeSelection.Items.Add(new RangeOption("20x range", InstallerEngine.RangeSelection.X20));
            _rangeSelection.SelectedIndex = 0;
            Controls.Add(_rangeSelection);

            var pakHelp = new Label
            {
                AutoSize = false,
                Location = new Point(220, 311),
                Size = new Size(470, 40),
                Text = "Choose exactly one multiplier. Setup safely replaces another owned Auto Pickup " +
                       "range PAK. Auto Pickup works independently of this choice."
            };
            Controls.Add(pakHelp);

            _status.Location = new Point(28, 356);
            _status.Size = new Size(666, 132);
            _status.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            _status.Multiline = true;
            _status.ReadOnly = true;
            _status.ScrollBars = ScrollBars.Vertical;
            _status.BackColor = SystemColors.Window;
            _status.Text = "Select the game executable to inspect the current installation.";
            Controls.Add(_status);

            _install.Location = new Point(535, 501);
            _install.Size = new Size(159, 30);
            _install.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            _install.Text = "Install";
            _install.Font = new Font(Font, FontStyle.Bold);
            _install.Enabled = false;
            _install.Click += InstallClick;
            Controls.Add(_install);

            _uninstall.Location = new Point(410, 501);
            _uninstall.Size = new Size(115, 30);
            _uninstall.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            _uninstall.Text = "Uninstall";
            _uninstall.Enabled = false;
            _uninstall.Click += UninstallClick;
            Controls.Add(_uninstall);

            _close.Location = new Point(310, 501);
            _close.Size = new Size(90, 30);
            _close.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            _close.Text = "Close";
            _close.Click += delegate { Close(); };
            Controls.Add(_close);

            AcceptButton = _install;
            CancelButton = _close;

            var discovered = InstallerEngine110.TryDiscoverGameExecutable();
            if (!string.IsNullOrWhiteSpace(discovered))
            {
                _gamePath.Text = discovered;
                RefreshInstallationState(true);
            }
        }

        private void BrowseClick(object sender, EventArgs e)
        {
            using (var dialog = new OpenFileDialog())
            {
                dialog.Title = "Select DSClient-Win64-Shipping.exe";
                dialog.Filter = "DragonSword executable (DSClient-Win64-Shipping.exe)|DSClient-Win64-Shipping.exe";
                dialog.CheckFileExists = true;
                dialog.Multiselect = false;
                if (dialog.ShowDialog(this) != DialogResult.OK) return;
                _gamePath.Text = dialog.FileName;
                RefreshInstallationState(true);
            }
        }

        private void RefreshInstallationState(bool selectDetectedOptions)
        {
            if (_busy) return;
            _installationState = null;
            try
            {
                var state = InstallerEngine110.InspectInstallationState(_gamePath.Text);
                _installationState = state;
                if (selectDetectedOptions)
                {
                    SelectInstalledRange(state.RangeState);
                    if (state.IsOwned)
                    {
                        _hotkey.Text = state.ToggleHotkey;
                        _interactionKeyFallback.Text = state.InteractionKeyFallback;
                    }
                }

                _status.Text = state.StatusDescription + Environment.NewLine +
                    "Optional range: " + DescribeRangeState(state.RangeState) + "." + Environment.NewLine +
                    (state.IsOwned
                        ? (state.CanRepair ? "Repair" : "Upgrade") +
                          " replaces installer-owned files in place and preserves config.ini. Uninstall removes only Auto Pickup and its owned range PAK."
                        : state.CanInstall
                            ? "Install is available. UE4SS is installed or converted only when required."
                            : "Setup will not modify files whose ownership cannot be verified.");
            }
            catch (Exception exception)
            {
                _status.Text = "Installation state could not be inspected." + Environment.NewLine + exception.Message;
            }
            UpdateActionButtons();
        }

        private void UpdateActionButtons()
        {
            var canInstall = _installationState != null &&
                (_installationState.CanInstall || _installationState.CanUpgrade ||
                 _installationState.CanRepair);
            _install.Text = _installationState != null && _installationState.CanRepair
                ? "Repair"
                : _installationState != null && _installationState.CanUpgrade
                    ? "Upgrade"
                    : "Install";
            _install.Enabled = !_busy && canInstall;
            _uninstall.Enabled = !_busy && _installationState != null && _installationState.CanUninstall;
            _browse.Enabled = !_busy;
            _gamePath.Enabled = !_busy;
            _hotkey.Enabled = !_busy;
            _interactionKeyFallback.Enabled = !_busy;
            _rangeSelection.Enabled = !_busy;
            _close.Enabled = !_busy;
        }

        private void InstallClick(object sender, EventArgs e)
        {
            RefreshInstallationState(false);
            if (_installationState == null ||
                (!_installationState.CanInstall && !_installationState.CanUpgrade &&
                 !_installationState.CanRepair))
            {
                MessageBox.Show(this, "Setup cannot install, upgrade, or repair the current installation safely.",
                    "Action unavailable", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            var requestedRepair = _installationState.CanRepair;

            SetBusy(true);
            try
            {
                var pakState = _installationState.RangeState;
                var requestedRange = SelectedRange;
                if (pakState != InstallerEngine.OptionalRangePakState.Absent &&
                    !StateMatchesSelection(pakState, requestedRange))
                {
                    var choice = MessageBox.Show(
                        this,
                        "The current owned range PAK selection will be replaced or removed. Continue?",
                        "Change optional range PAK?",
                        MessageBoxButtons.YesNo,
                        MessageBoxIcon.Warning,
                        MessageBoxDefaultButton.Button2);
                    if (choice != DialogResult.Yes)
                    {
                        _status.Text = "Action cancelled. No files were changed.";
                        return;
                    }
                }

                _status.Text = "Validating the game, UE4SS, and embedded release payload...";
                _status.Refresh();

                var plan = InstallerEngine110.Inspect(
                    _gamePath.Text,
                    _hotkey.Text,
                    _interactionKeyFallback.Text,
                    requestedRange);
                if (plan.ConvertsUE4SS)
                {
                    var conversion = MessageBox.Show(
                        this,
                        "A different or mixed UE4SS installation was detected.\n\n" +
                        plan.ActionDescription + "\n\nExisting layout: " + plan.ExistingLayout +
                        "\n\nContinue with the verified conversion?",
                        "Convert UE4SS installation?",
                        MessageBoxButtons.YesNo,
                        MessageBoxIcon.Warning,
                        MessageBoxDefaultButton.Button2);
                    if (conversion != DialogResult.Yes)
                    {
                        _status.Text = "Action cancelled. No files were changed.";
                        return;
                    }
                }

                var result = InstallerEngine110.InstallConfirmed(
                    _gamePath.Text,
                    _hotkey.Text,
                    _interactionKeyFallback.Text,
                    requestedRange,
                    plan.IdentityToken);

                var verb = requestedRepair
                    ? "Repair"
                    : plan.UpdatesExistingAutoPickup ? "Upgrade" : "Installation";
                _status.Text =
                    verb + " completed successfully." + Environment.NewLine +
                    "UE4SS layout: " + result.LayoutDescription + Environment.NewLine +
                    "Toggle key: " + result.ToggleHotkey + Environment.NewLine +
                    "Interaction fallback: " + result.InteractionKeyFallback + Environment.NewLine +
                    "Optional range: " + (result.RangeMultiplierInstalled == 0
                        ? "original"
                        : result.RangeMultiplierInstalled + "x") +
                    (string.IsNullOrWhiteSpace(result.BackupDirectory)
                        ? string.Empty
                        : Environment.NewLine + "UE4SS conversion backup: " + result.BackupDirectory);

                MessageBox.Show(
                    this,
                    verb + " completed. Auto Pickup starts off each time the game starts. Press " +
                    result.ToggleHotkey + " to enable it.",
                    verb + " complete",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
            }
            catch (Exception exception)
            {
                _status.Text = "Action stopped safely." + Environment.NewLine + exception.Message;
                MessageBox.Show(this, exception.Message, "Action stopped", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                SetBusy(false);
                RefreshInstallationState(true);
            }
        }

        private void UninstallClick(object sender, EventArgs e)
        {
            RefreshInstallationState(false);
            if (_installationState == null || !_installationState.CanUninstall)
            {
                MessageBox.Show(this, "No installer-owned Auto Pickup installation is available to remove.",
                    "Uninstall unavailable", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            var confirmation = MessageBox.Show(
                this,
                "Remove Auto Pickup and its owned optional range PAK?\n\n" +
                "UE4SS, other Mods, and unrelated files will be preserved.",
                "Uninstall Auto Pickup?",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning,
                MessageBoxDefaultButton.Button2);
            if (confirmation != DialogResult.Yes) return;

            var identity = _installationState.IdentityToken;
            SetBusy(true);
            try
            {
                var result = InstallerEngine110.UninstallConfirmed(_gamePath.Text, identity);
                _status.Text = "Auto Pickup was removed successfully." +
                    (result.RemovedRangePak ? " Its owned optional range PAK was also removed." : string.Empty) +
                    Environment.NewLine + "UE4SS and unrelated Mods were preserved.";
                MessageBox.Show(this, "Auto Pickup was removed successfully.", "Uninstall complete",
                    MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch (Exception exception)
            {
                _status.Text = "Uninstall stopped safely." + Environment.NewLine + exception.Message;
                MessageBox.Show(this, exception.Message, "Uninstall stopped", MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        private InstallerEngine.RangeSelection SelectedRange
        {
            get
            {
                var option = _rangeSelection.SelectedItem as RangeOption;
                return option == null ? InstallerEngine.RangeSelection.None : option.Selection;
            }
        }

        private void SelectInstalledRange(InstallerEngine.OptionalRangePakState state)
        {
            var selection = InstallerEngine.RangeSelection.None;
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX3) selection = InstallerEngine.RangeSelection.X3;
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX5) selection = InstallerEngine.RangeSelection.X5;
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX10) selection = InstallerEngine.RangeSelection.X10;
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX15) selection = InstallerEngine.RangeSelection.X15;
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX20) selection = InstallerEngine.RangeSelection.X20;
            for (var index = 0; index < _rangeSelection.Items.Count; index++)
            {
                var option = _rangeSelection.Items[index] as RangeOption;
                if (option != null && option.Selection == selection)
                {
                    _rangeSelection.SelectedIndex = index;
                    return;
                }
            }
        }

        private static bool StateMatchesSelection(
            InstallerEngine.OptionalRangePakState state,
            InstallerEngine.RangeSelection selection)
        {
            return (state == InstallerEngine.OptionalRangePakState.Absent && selection == InstallerEngine.RangeSelection.None) ||
                   (state == InstallerEngine.OptionalRangePakState.ApprovedX3 && selection == InstallerEngine.RangeSelection.X3) ||
                   (state == InstallerEngine.OptionalRangePakState.ApprovedX5 && selection == InstallerEngine.RangeSelection.X5) ||
                   (state == InstallerEngine.OptionalRangePakState.ApprovedX10 && selection == InstallerEngine.RangeSelection.X10) ||
                   (state == InstallerEngine.OptionalRangePakState.ApprovedX15 && selection == InstallerEngine.RangeSelection.X15) ||
                   (state == InstallerEngine.OptionalRangePakState.ApprovedX20 && selection == InstallerEngine.RangeSelection.X20);
        }

        private static string DescribeRangeState(InstallerEngine.OptionalRangePakState state)
        {
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX3) return "3x";
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX5) return "5x";
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX10) return "10x";
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX15) return "15x";
            if (state == InstallerEngine.OptionalRangePakState.ApprovedX20) return "20x";
            if (state == InstallerEngine.OptionalRangePakState.MultipleApproved)
                return "multiple owned PAKs (choose one to reconcile)";
            return "original";
        }

        private sealed class RangeOption
        {
            internal RangeOption(string label, InstallerEngine.RangeSelection selection)
            {
                Label = label;
                Selection = selection;
            }

            internal string Label { get; private set; }
            internal InstallerEngine.RangeSelection Selection { get; private set; }

            public override string ToString()
            {
                return Label;
            }
        }
    }
}
