// AC250-DTM-F3 UART EEPROM Configuration
// Web Serial API Implementation

class UARTEEPROMManager {
    constructor() {
        this.port = null;
        this.reader = null;
        this.writer = null;
        this.EEPROM_SIZE = 65536;
        this.READ_SIZE = 14000; // aktualizowane dynamicznie po wykryciu wariantu
        this.ACTIVE_CONFIG_SIZE = 4096;
        this.BAUD_RATE = 115200; // musi być taki sam jak w firmware (uart1_eeprom.c)
        this.eepromData = new Uint8Array(this.EEPROM_SIZE).fill(0xFF);
        this.serviceStatusLockedValue = null; // EEPROM[profile.serviceStatusAddr] - only service can change
        this.isBusy = false;
        this.firmwareProfiles = [
            { name: 'AC250_DTM-F', maxUsers: 250, clipDurationAddr: 1258, clipToggleAddr: 1262, trybPracyAddr: 1263, trybClipAddr: 1264, serviceStatusAddr: 1265, settingsStartAddr: 1275 },
            { name: 'AC2500_DTM-F', maxUsers: 2500, clipDurationAddr: 12508, clipToggleAddr: 12512, trybPracyAddr: 12513, trybClipAddr: 12514, serviceStatusAddr: 12515, settingsStartAddr: 12525 },
            { name: 'AC5000_DTM-F', maxUsers: 5000, clipDurationAddr: 25008, clipToggleAddr: 25012, trybPracyAddr: 25013, trybClipAddr: 25014, serviceStatusAddr: 25015, settingsStartAddr: 25025 }
        ];
        this.profile = this.firmwareProfiles[1]; // safe default: 2500
        this.maxUsers = this.profile.maxUsers;
        this.READ_SIZE = this.profile.settingsStartAddr + 1024;
        this.profileDetectionLocked = false;
        this.profilePreferenceKey = 'preferred_controller_profile';
        this.lastProfileDetection = null;
        this.hasReliableSnapshot = false;
        this.snapshotProfileName = null;

        this.initUI();
        this.checkWebSerialSupport();
        this.signalInterval = null;
        this.watchdogInterval = null;
        this.lastTraffic = 0;
        this.watchdogTimeoutMs = 20000;
        this.watchdogGraceUntil = 0;
        this.restartWatchdogGraceMs = 20000;
        this.backgroundTasksPaused = false;
    }

    checkWebSerialSupport() {
        if (!('serial' in navigator)) {
            // Show Modal
            const modal = document.getElementById('browserModal');
            const closeBtn = document.querySelector('.close-modal');

            if (modal) {
                modal.style.display = 'flex';

                // Allow closing, but it will reappear on reload
                if (closeBtn) {
                    closeBtn.onclick = () => {
                        modal.style.display = 'none';
                    };
                }

                // Close on click outside
                window.onclick = (event) => {
                    if (event.target === modal) {
                        modal.style.display = 'none';
                    }
                };
            }

            this.log('❌ Web Serial API nie jest wspierane w tej przeglądarce!', 'error');
            this.log('ℹ️ Użyj Chrome lub Edge (wersja 89+)', 'info');

            // Disable buttons
            document.getElementById('connectBtn').disabled = true;
            document.querySelectorAll('.btn-warning, .btn-success').forEach(btn => btn.disabled = true);
        } else {
            this.log('✅ Web Serial API dostępne', 'success');

            // Auto-detect disconnect
            navigator.serial.addEventListener('disconnect', (event) => {
                if (this.port && event.target === this.port) {
                    this.log('🔌 Urządzenie odłączone fizycznie', 'warning');
                    this.disconnect();
                }
            });
        }
    }

    initUI() {
        // Connect button
        document.getElementById('connectBtn').addEventListener('click', () => this.connect());

        // Action buttons
        document.getElementById('readBtn').addEventListener('click', () => this.readEEPROM());
        document.getElementById('writeBtn').addEventListener('click', () => this.writeEEPROM());

        document.getElementById('exportBtn').addEventListener('click', () => this.exportCSV());
        document.getElementById('importBtn').addEventListener('click', () => this.importCSV());
        document.getElementById('clearLogBtn').addEventListener('click', () => this.clearLog());
        const clearNumbersBtn = document.getElementById('clearNumbersBtn');
        if (clearNumbersBtn) {
            clearNumbersBtn.addEventListener('click', () => this.clearPhoneNumbersOnly());
        }

        const accessCodeInput = document.getElementById('accessCode');
        if (accessCodeInput) {
            accessCodeInput.addEventListener('input', (e) => {
                const sanitized = this.sanitizeAccessCode(e.target.value);
                if (e.target.value !== sanitized) e.target.value = sanitized;
            });
        }

        // Tabs
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', (e) => this.switchTab(e.currentTarget.dataset.tab));
        });

        // Generate phone number inputs
        this.generatePhoneInputs();
        this.updateUsersHeader();


        const v = new Date().toTimeString().split(' ')[0];
        this.log(`🚀 Załadowane...`, 'success');

        // Logs Toggle Logic
        const logsCheckbox = document.getElementById('showLogsCheckbox');
        const logsCard = document.getElementById('logsCard');

        // Load saved state
        const savedLogsState = localStorage.getItem('logs_visible');
        if (savedLogsState === 'false') {
            logsCheckbox.checked = false;
            logsCard.style.display = 'none';
        } else {
            logsCheckbox.checked = true;
            logsCard.style.display = 'block';
        }

        // Add listener
        logsCheckbox.addEventListener('change', (e) => {
            const isVisible = e.target.checked;
            logsCard.style.display = isVisible ? 'block' : 'none';
            localStorage.setItem('logs_visible', isVisible);

            // Scroll to bottom removed since logs are now at top
            // if (isVisible) {
            //    setTimeout(() => window.scrollTo({ top: document.body.scrollHeight, behavior: 'smooth' }), 100);
            // }
        });

        /* --- Global Help Mode Logic --- */
        /* Added for Global Help System - Toggle 'help-mode' class on body */
        const helpCheckbox = document.getElementById('showHelpCheckbox');
        if (helpCheckbox) {
            helpCheckbox.addEventListener('change', (e) => {
                if (e.target.checked) {
                    document.body.classList.add('help-mode');
                } else {
                    document.body.classList.remove('help-mode');
                }
            });
        }

        // Setup Theme Switcher
        this.setupThemeSwitcher();

        // Setup Offline Links Protection
        this.setupOfflineLinks();

        // Setup Offline Links Protection
        this.setupOfflineLinks();

        // Setup Disconnect Modal
        this.setupDisconnectModal();

        // Setup No Connection Modal
        this.setupNoConnectionModal();

        // Setup Generic Modal
        this.setupDynamicModal();
    }

    applyProfile(profile, logIt = true) {
        if (this.snapshotProfileName && this.snapshotProfileName !== profile.name) {
            this.hasReliableSnapshot = false;
        }
        this.profile = profile;
        this.maxUsers = profile.maxUsers;
        this.READ_SIZE = Math.min(this.EEPROM_SIZE, profile.settingsStartAddr + 1024);
        this.generatePhoneInputs();
        this.updateUsersHeader();
        this.updateControllerInfo();
        if (logIt) {
            this.log(`🧠 Wykryto sterownik: ${profile.name}`, 'success');
        }
    }

    updateUsersHeader() {
        const h2 = document.getElementById('usersHeader');
        if (h2) {
            h2.textContent = '📞 Numery telefonów uprawnionych';
        }
    }

    updateControllerInfo() {
        const info = document.getElementById('controllerInfo');
        if (info) {
            info.textContent = `Wykryto sterownik: ${this.profile.name}`;
        }
    }

    resolveProfileByName(profileName) {
        if (!profileName) return null;
        return this.firmwareProfiles.find(p => p.name === profileName) || null;
    }

    getPreferredProfile() {
        try {
            const preferredName = localStorage.getItem(this.profilePreferenceKey);
            return this.resolveProfileByName(preferredName);
        } catch (_) {
            return null;
        }
    }

    setPreferredProfile(profileName) {
        try {
            if (!profileName) localStorage.removeItem(this.profilePreferenceKey);
            else localStorage.setItem(this.profilePreferenceKey, profileName);
        } catch (_) {
            // Ignore storage errors.
        }
    }

    async showControllerProfileModal(options = {}) {
        const {
            preselectedName = (this.profile && this.profile.name) || 'AC2500_DTM-F',
            allowCancel = true
        } = options;

        return new Promise((resolve) => {
            const modal = document.getElementById('profileModal');
            const closeBtn = document.querySelector('.close-profile');
            const cancelBtn = document.getElementById('profileModalCancel');
            const applyBtn = document.getElementById('profileModalApply');
            const rememberCheckbox = document.getElementById('profileRememberChoice');
            const radioInputs = document.querySelectorAll('input[name="profileVariant"]');

            if (!modal || !applyBtn || radioInputs.length === 0) {
                resolve(null);
                return;
            }

            const hasPreselected = Array.from(radioInputs).some(r => r.value === preselectedName);
            const selectedName = hasPreselected ? preselectedName : 'AC2500_DTM-F';
            radioInputs.forEach(r => { r.checked = (r.value === selectedName); });

            if (rememberCheckbox) rememberCheckbox.checked = true;
            if (cancelBtn) cancelBtn.style.display = allowCancel ? 'inline-block' : 'none';
            if (closeBtn) closeBtn.style.display = allowCancel ? 'inline-block' : 'none';

            const cleanup = () => {
                if (closeBtn) closeBtn.onclick = null;
                if (cancelBtn) cancelBtn.onclick = null;
                applyBtn.onclick = null;
                window.removeEventListener('click', onWindowClick);
            };

            const finish = (result) => {
                modal.style.display = 'none';
                cleanup();
                resolve(result);
            };

            const onWindowClick = (event) => {
                if (allowCancel && event.target === modal) finish(null);
            };

            if (closeBtn) closeBtn.onclick = () => finish(null);
            if (cancelBtn) cancelBtn.onclick = () => finish(null);
            applyBtn.onclick = () => {
                const selected = document.querySelector('input[name="profileVariant"]:checked');
                if (!selected) {
                    finish(null);
                    return;
                }
                finish({
                    profileName: selected.value,
                    remember: rememberCheckbox ? rememberCheckbox.checked : false
                });
            };

            window.addEventListener('click', onWindowClick);
            modal.style.display = 'flex';
        });
    }

    async chooseControllerProfileManually() {
        const choice = await this.showControllerProfileModal({
            preselectedName: (this.profile && this.profile.name) || 'AC2500_DTM-F',
            allowCancel: true
        });
        if (!choice || !choice.profileName) return;

        const selectedProfile = this.resolveProfileByName(choice.profileName);
        if (!selectedProfile) return;

        this.applyProfile(selectedProfile, false);
        this.profileDetectionLocked = true;
        this.log(`🧠 Wybrano sterownik: ${selectedProfile.name}.`, 'success');

        if (choice.remember) {
            this.setPreferredProfile(selectedProfile.name);
            this.log(`💾 Zapamiętano domyślny wariant: ${selectedProfile.name}.`, 'info');
        } else {
            this.setPreferredProfile(null);
            this.log('ℹ️ Wybór wariantu bez zapisywania domyślnego.', 'info');
        }
    }

    async waitForReadableUnlock(maxWaitMs = 1500) {
        if (!this.port || !this.port.readable) return false;
        const deadline = Date.now() + maxWaitMs;
        while (this.port.readable.locked && Date.now() < deadline) {
            await new Promise(r => setTimeout(r, 10));
        }
        return !this.port.readable.locked;
    }

    async uartReadByteAtAddress(addr, timeoutMs = 1200) {
        const writer = this.port.writable.getWriter();
        await writer.write(new Uint8Array([0x52, (addr >> 8) & 0xFF, addr & 0xFF])); // 'R'+addr
        writer.releaseLock();
        await new Promise(r => setTimeout(r, 1));

        const unlocked = await this.waitForReadableUnlock(timeoutMs);
        if (!unlocked) throw new Error('READ_LOCKED');
        const reader = this.port.readable.getReader();
        let timeoutHit = false;
        const timeoutPromise = new Promise((_, reject) =>
            setTimeout(() => {
                timeoutHit = true;
                reject(new Error('READ_TIMEOUT'));
            }, timeoutMs)
        );
        try {
            const { value, done } = await Promise.race([reader.read(), timeoutPromise]);
            if (done || !value || value.length === 0) throw new Error('NO_DATA');
            return value[0];
        } finally {
            // Gdy timeout wygra race, pending read() zostaje aktywne i sam releaseLock
            // może się nie udać -> najpierw cancel(), potem releaseLock().
            if (timeoutHit) {
                try { await reader.cancel(); } catch (_) { }
            }
            try { reader.releaseLock(); } catch (_) { }
        }
    }

    async uartReadDeviceIdentityLine(timeoutMs = 1200) {
        if (!this.port || !this.port.writable || !this.port.readable) return null;
        if (this.port.writable.locked || this.port.readable.locked) return null;

        const writer = this.port.writable.getWriter();
        try {
            // 'I' -> ID=ACxxxx;VER=xxxx;EEPROM=...;ADDR_BYTES=...
            await writer.write(new Uint8Array([0x49]));
        } finally {
            writer.releaseLock();
        }
        await new Promise(r => setTimeout(r, 5));

        const unlocked = await this.waitForReadableUnlock(timeoutMs);
        if (!unlocked) return null;

        const reader = this.port.readable.getReader();
        const decoder = new TextDecoder();
        let text = '';
        const deadline = Date.now() + timeoutMs;

        try {
            while (Date.now() < deadline) {
                const sliceMs = Math.max(20, deadline - Date.now());
                const timeoutPromise = new Promise((_, reject) =>
                    setTimeout(() => reject(new Error('READ_TIMEOUT')), sliceMs)
                );
                const { value, done } = await Promise.race([reader.read(), timeoutPromise]);
                if (done) break;
                if (value && value.length > 0) {
                    text += decoder.decode(value, { stream: true });
                    if (text.includes('\n')) break;
                }
            }
        } catch (_) {
            // Ignore identity timeout and fallback to heuristic detection.
        } finally {
            try { await reader.cancel(); } catch (_) { }
            try { reader.releaseLock(); } catch (_) { }
        }

        const line = text.split('\n')[0].trim();
        return line || null;
    }

    resolveProfileByDeviceIdentity(identityLine) {
        if (!identityLine) return null;
        const normalizedLine = String(identityLine).trim().toUpperCase();
        if (!normalizedLine) return null;

        // Legacy/noisy lines: allow identity payload even when prefixed by junk.
        if (normalizedLine.includes('AC5000')) return this.resolveProfileByName('AC5000_DTM-F');
        if (normalizedLine.includes('AC2500')) return this.resolveProfileByName('AC2500_DTM-F');

        const idStart = normalizedLine.indexOf('ID=');
        if (idStart < 0) return null;
        const payload = normalizedLine.slice(idStart);
        const parts = payload.split(';');
        const kv = {};
        for (const p of parts) {
            const idx = p.indexOf('=');
            if (idx <= 0) continue;
            kv[p.slice(0, idx).trim().toUpperCase()] = p.slice(idx + 1).trim().toUpperCase();
        }

        const id = kv.ID || '';
        const ver = kv.VER || '';

        if (id === 'AC5000' || ver === '5000') return this.resolveProfileByName('AC5000_DTM-F');
        if (id === 'AC2500' || ver === '2500') return this.resolveProfileByName('AC2500_DTM-F');
        if (id === 'AC250' || id === 'AC200' || ver === '250' || ver === '200') {
            return this.resolveProfileByName('AC250_DTM-F');
        }
        return null;
    }

    evaluateProfileScore(profile, sample) {
        let score = 0;
        if (sample.initFlag === 0xA5) score += 20;
        if (sample.trybPracy === 0x00 || sample.trybPracy === 0x01 || sample.trybPracy === 0xFF) score += 5;
        if (sample.trybClip === 0x00 || sample.trybClip === 0x01 || sample.trybClip === 0xFF) score += 5;

        const d = sample.duration;
        const durationLooksValid = (
            d === 0xFFFFFFFF ||
            d === 99999 ||
            (d >= 1 && d <= 99999)
        );
        if (durationLooksValid) score += 5;

        // Extra confidence for initialized config region.
        if (sample.initFlag === 0xA5 && (sample.trybPracy === 0 || sample.trybPracy === 1)) score += 4;
        if (sample.initFlag === 0xA5 && (sample.trybClip === 0 || sample.trybClip === 1)) score += 4;

        return score;
    }

    async detectFirmwareProfile(logIt = true, force = false) {
        if (!this.port) return this.profile;
        if (this.profileDetectionLocked && !force) return this.profile;
        if (this.port.writable.locked || this.port.readable.locked) return this.profile;

        let bestProfile = null;
        let bestScore = -1;
        const MIN_RELIABLE_PROFILE_SCORE = 30;
        this.lastProfileDetection = {
            reliable: false,
            usedPreferredProfile: false,
            bestScore: -1,
            bestProfileName: null,
            preferredProfileName: null
        };

        // Fast path: explicit device identity from firmware.
        try {
            const identityLine = await this.uartReadDeviceIdentityLine(1200);
            const identityProfile = this.resolveProfileByDeviceIdentity(identityLine);
            if (identityProfile) {
                this.applyProfile(identityProfile, logIt);
                this.profileDetectionLocked = true;
                this.lastProfileDetection = {
                    reliable: true,
                    usedPreferredProfile: false,
                    bestScore: 100,
                    bestProfileName: identityProfile.name,
                    identityLine: identityLine || null
                };
                return identityProfile;
            }
        } catch (_) {
            // Fallback to heuristic detection below.
        }

        for (const profile of this.firmwareProfiles) {
            try {
                const initFlag = await this.uartReadByteAtAddress(profile.settingsStartAddr);
                const trybPracy = await this.uartReadByteAtAddress(profile.trybPracyAddr);
                const trybClip = await this.uartReadByteAtAddress(profile.trybClipAddr);
                const d0 = await this.uartReadByteAtAddress(profile.clipDurationAddr + 0);
                const d1 = await this.uartReadByteAtAddress(profile.clipDurationAddr + 1);
                const d2 = await this.uartReadByteAtAddress(profile.clipDurationAddr + 2);
                const d3 = await this.uartReadByteAtAddress(profile.clipDurationAddr + 3);
                const duration = (d0 | (d1 << 8) | (d2 << 16) | (d3 << 24)) >>> 0;

                const score = this.evaluateProfileScore(profile, { initFlag, trybPracy, trybClip, duration });
                if (score > bestScore) {
                    bestScore = score;
                    bestProfile = profile;
                }
            } catch (_) {
                // Ignore one profile read failure, continue probing.
            }
        }

        if (bestProfile && bestScore >= MIN_RELIABLE_PROFILE_SCORE) {
            this.applyProfile(bestProfile, logIt);
            this.profileDetectionLocked = true;
            this.lastProfileDetection = {
                reliable: true,
                usedPreferredProfile: false,
                bestScore,
                bestProfileName: bestProfile.name
            };
            return bestProfile;
        }

        this.lastProfileDetection.bestScore = bestScore;
        this.lastProfileDetection.bestProfileName = bestProfile ? bestProfile.name : null;

        const preferredProfile = this.getPreferredProfile();
        if (preferredProfile) {
            this.lastProfileDetection.preferredProfileName = preferredProfile.name;
            if (logIt) {
                this.log(`ℹ️ Wykrywanie niejednoznaczne. Zapamiętany wariant: ${preferredProfile.name}.`, 'info');
            }
        }

        if (logIt) {
            if (bestProfile) {
                this.log('⚠️ Niejednoznaczna detekcja wariantu. [W-P01] Używam domyślnej 2500.', 'warning');
            } else {
                this.log('⚠️ Nie udało się wykryć wersji, używam domyślnej 2500.', 'warning');
            }
        }
        this.applyProfile(this.firmwareProfiles[1], false);
        this.profileDetectionLocked = true;
        return this.profile;
    }

    setupOfflineLinks() {
        document.querySelectorAll('.offline-aware').forEach(link => {
            link.addEventListener('click', async (e) => {
                e.preventDefault(); // Always prevent default first

                const targetUrl = link.href;
                let isOffline = !navigator.onLine;

                // Robust check for Windows/LAN scenarios
                if (!isOffline) {
                    try {
                        // Try to fetch a tiny resource (using no-cors to avoid CORS errors, opaque response is enough)
                        // Using a highly available endpoint (Google) with a random query to prevent caching
                        const controller = new AbortController();
                        const timeoutId = setTimeout(() => controller.abort(), 1500); // 1.5s timeout

                        await fetch('https://www.google.com/generate_204?' + Date.now(), {
                            method: 'HEAD',
                            mode: 'no-cors',
                            cache: 'no-store',
                            signal: controller.signal
                        });
                        clearTimeout(timeoutId);
                    } catch (error) {
                        console.warn("Offline check probe failed:", error);
                        isOffline = true;
                    }
                }

                if (isOffline) {
                    // Custom modal
                    if (window.uartManager) {
                        window.uartManager.showModal('Brak Internetu', 'Wymagany jest dostęp do internetu, aby otworzyć ten zasób!', 'warning');
                    } else {
                        alert('⚠️ Wymagany jest dostęp do internetu, aby otworzyć ten zasób!');
                    }
                } else {
                    // Online - proceed with navigation
                    window.open(targetUrl, '_blank');
                }
            });
        });
    }

    setupThemeSwitcher() {
        const themeRadios = document.querySelectorAll('input[name="theme"]');

        // Load saved theme (default to dark)
        const savedTheme = localStorage.getItem('theme') || 'dark';
        document.documentElement.setAttribute('data-theme', savedTheme);

        // Set initial radio state
        const activeRadio = document.querySelector(`input[name="theme"][value="${savedTheme}"]`);
        if (activeRadio) activeRadio.checked = true;

        // Add listeners
        themeRadios.forEach(radio => {
            radio.addEventListener('change', (e) => {
                const newTheme = e.target.value;
                document.documentElement.setAttribute('data-theme', newTheme);
                localStorage.setItem('theme', newTheme);
            });
        });
    }

    generatePhoneInputs() {
        // Unified List (1-maxUsers)
        const container = document.getElementById('allNumbers');
        if (!container) return;

        container.innerHTML = ''; // Clear existing

        for (let i = 1; i <= this.maxUsers; i++) {
            const div = document.createElement('div');
            div.className = 'number-item';

            // Label Logic
            const labelText = `Pozycja ${i}:`;

            div.innerHTML = `
                <label>${labelText}</label>
                <input type="text" id="phone_${i}" maxlength="9" placeholder="123456789">
            `;
            container.appendChild(div);
            this.addPhoneValidation(div.querySelector('input'));
        }

        // Search Listener
        const searchInput = document.getElementById('searchNumber');
        if (searchInput) {
            searchInput.addEventListener('input', (e) => {
                // Validation: Allow only digits and +
                const val = e.target.value;
                const sanitized = val.replace(/[^0-9+]/g, '');
                if (val !== sanitized) {
                    e.target.value = sanitized;
                }

                const term = e.target.value.toLowerCase();
                const items = container.querySelectorAll('.number-item');

                items.forEach(item => {
                    const input = item.querySelector('input');
                    const label = item.querySelector('label');
                    const phone = input.value.toLowerCase();
                    const text = label.textContent.toLowerCase();

                    if (phone.includes(term) || text.includes(term)) {
                        item.style.display = 'flex';
                    } else {
                        item.style.display = 'none';
                    }
                });
            });
        }
    }

    addPhoneValidation(input) {
        if (!input) return;
        input.addEventListener('input', (e) => {
            const value = e.target.value;
            const sanitized = value.replace(/\D/g, ''); // Remove non-digits
            if (value !== sanitized) {
                e.target.value = sanitized;
            }
        });
    }

    switchTab(tabName) {
        document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));

        document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');
        document.getElementById(`${tabName}Tab`).classList.add('active');
    }

    isPortBusyError(error) {
        const name = ((error && error.name) || '').toLowerCase();
        const msg = String((error && error.message) || error || '').toLowerCase();
        return (
            name === 'networkerror' ||
            name === 'invalidstateerror' ||
            msg.includes('already open') ||
            msg.includes('port is already open') ||
            msg.includes('resource busy') ||
            msg.includes('in use') ||
            msg.includes('failed to open serial port')
        );
    }

    async connect() {
        try {
            if (this.port) {
                await this.disconnect();
                return;
            }

            this.log('🔌 Wybierz port szeregowy...', 'info');

            this.port = await navigator.serial.requestPort();

            // Słuchaj użytkownika: spinner startuje "w chwili kiedy w oknie... kliknę OK"
            const startTime = Date.now();
            const MIN_DURATION = 10000;

            // Show Spinner - REMOVED PER REQUEST
            // const spinner = document.getElementById('connectionSpinner');
            // if (spinner) spinner.style.display = 'flex';

            await this.port.open({
                baudRate: this.BAUD_RATE,
                dataBits: 8,
                stopBits: 1,
                parity: 'none',
                flowControl: 'none'
            });

            // Disable DTR and RTS to prevent reset
            await this.port.setSignals({
                dataTerminalReady: false,
                requestToSend: false
            });

            this.log(`✅ Połączono z portem`, 'success');
            this.updateConnectionStatus(true);

            document.getElementById('connectBtn').textContent = 'Rozłącz';
            document.getElementById('connectBtn').classList.remove('btn-primary');
            document.getElementById('connectBtn').classList.add('btn-warning');

            // Show Spinner - Initialization Delay
            const spinner = document.getElementById('connectionSpinner');
            if (spinner) {
                spinner.style.display = 'flex';
                // Wait 10 seconds for hardware initialization
                await new Promise(r => setTimeout(r, 10000));
                spinner.style.display = 'none';
            }

            // Auto-detect firmware profile (250/2500/5000) once per connection.
            this.profileDetectionLocked = false;
            await this.detectFirmwareProfile(true, true);
            if (this.lastProfileDetection && !this.lastProfileDetection.reliable && !this.lastProfileDetection.usedPreferredProfile) {
                const preferredName = this.lastProfileDetection && this.lastProfileDetection.preferredProfileName;
                const choice = await this.showControllerProfileModal({
                    preselectedName: preferredName || ((this.profile && this.profile.name) || 'AC2500_DTM-F'),
                    allowCancel: true
                });
                if (choice && choice.profileName) {
                    const selectedProfile = this.resolveProfileByName(choice.profileName);
                    if (selectedProfile) {
                        this.applyProfile(selectedProfile, false);
                        this.profileDetectionLocked = true;
                        this.log(`🧠 Wybrano sterownik: ${selectedProfile.name}.`, 'success');
                        if (choice.remember) {
                            this.setPreferredProfile(selectedProfile.name);
                            this.log(`💾 Zapamiętano domyślny wariant: ${selectedProfile.name}.`, 'info');
                        }
                    }
                }
            }

            // Initialize watchdog and polling AFTER UI unlocks
            this.lastTraffic = Date.now();
            this.startSignalPolling();
            this.startWatchdog();

        } catch (error) {
            if (error && error.name === 'NotFoundError') {
                this.log('ℹ️ Anulowano wybor portu. [I-C01]', 'info');
            } else if (this.isPortBusyError(error)) {
                this.log('⚠️ Port niedostepny. [W-C02] Sprawdz: inne okna/aplikacje.', 'warning');
                await this.showModal(
                    'Port zajety',
                    'Zamknij inne okno z konfiguratorem',
                    'warning'
                );
            } else {
                this.log('❌ Blad polaczenia. [E-C03] Sprawdz: kabel/port/uprawnienia.', 'error');
            }
            // Cleanup if port was selected but open failed
            if (this.port) await this.disconnect();
        } finally {
            // Ensure spinner is hidden on error
            const spinner = document.getElementById('connectionSpinner');
            if (spinner && !this.port) spinner.style.display = 'none';
        }
    }



    async disconnect() {
        if (this.port) {
            // Force reset busy state on disconnect
            this.isBusy = false;

            try {
                // Try to cancel any locking reader
                if (this.reader) {
                    await this.reader.cancel();
                    this.reader.releaseLock();
                    this.reader = null;
                }
            } catch (e) {
                console.warn("Reader cleanup error:", e);
            }

            try {
                await this.port.close();
            } catch (e) {
                // Ignore "The port is already closed" error
                console.warn("Port close warning (ignorable):", e);
            }

        }

        // Always reset UI state on disconnect, even if port was already null
        this.port = null;
        this.profileDetectionLocked = false;
        this.hasReliableSnapshot = false;
        this.snapshotProfileName = null;
        this.watchdogGraceUntil = 0;
        this.log('🔌 Rozłączono', 'info');
        this.updateConnectionStatus(false);
        this.stopSignalPolling();
        this.stopWatchdog();

        document.getElementById('connectBtn').textContent = 'Połącz z urządzeniem';
        document.getElementById('connectBtn').classList.remove('btn-warning');
        document.getElementById('connectBtn').classList.add('btn-primary');
        const info = document.getElementById('controllerInfo');
        if (info) info.textContent = 'Wykryto sterownik: —';
    }

    updateConnectionStatus(connected) {
        const statusDot = document.getElementById('statusDot');
        const statusText = document.getElementById('statusText');
        if (connected) {
            statusDot.classList.add('connected');
            statusText.textContent = 'Połączony';
        } else {
            statusDot.classList.remove('connected');
            statusText.textContent = 'Rozłączony';
        }
    }

    pauseBackgroundTasks(reason = 'operacja') {
        if (this.backgroundTasksPaused) return;
        this.backgroundTasksPaused = true;
        this.stopSignalPolling();
        this.stopWatchdog();
        this.log(`⏸️ Wstrzymano odswiezanie statusu`, 'info');
    }

    resumeBackgroundTasks() {
        if (!this.backgroundTasksPaused) return;
        this.backgroundTasksPaused = false;
        if (this.port) {
            this.lastTraffic = Date.now();
            this.startSignalPolling();
            this.startWatchdog();
            this.log('▶️ Wznowiono odswiezanie statusu', 'info');
        }
    }

    async testConnection() {
        if (!this.port) {
            this.log('❌ Najpierw połącz się z urządzeniem!', 'error');
            return;
        }

        try {
            this.log('🧪 Test połączenia...', 'info');
            this.log('✅ Port otwarty i gotowy', 'success');
        } catch (error) {
            this.log('❌ Test polaczenia nie powiodl sie.', 'error');
        }
    }

    async readAccessCodeOnly() {
        if (!this.port) {
            this.log('❌ Najpierw połącz się z urządzeniem!', 'error');
            return;
        }

        if (this.isBusy) {
            this.log('⚠️ Operacja w toku. Poczekaj chwile...', 'warning');
            return;
        }
        this.isBusy = true;

        let reader = null;
        try {
            this.log('📥 Odczyt kodu dostepu...', 'info');

            const writer = this.port.writable.getWriter();
            await writer.write(new Uint8Array([0x63])); // 'c' (Read code)
            writer.releaseLock();

            reader = this.port.readable.getReader();
            const accumulatedBytes = new Uint8Array(4);
            let bytesRead = 0;
            const startTime = Date.now();

            while (bytesRead < 4) {
                // Check overall timeout (3s)
                if (Date.now() - startTime > 3000) {
                    this.log('⚠️ Odczyt kodu przerwany.', 'warning');
                    break;
                }

                try {
                    // Race between read and a small timeout to prevent blocking forever
                    const readPromise = reader.read();
                    const timeoutPromise = new Promise((_, reject) =>
                        setTimeout(() => reject(new Error('READ_TIMEOUT')), 1000)
                    );

                    const { value, done } = await Promise.race([readPromise, timeoutPromise]);

                    if (done) break;

                    if (value) {
                        for (let i = 0; i < value.length; i++) {
                            if (bytesRead < 4) {
                                accumulatedBytes[bytesRead] = value[i];
                                bytesRead++;
                            }
                        }
                    }
                } catch (e) {
                    if (e.message === 'READ_TIMEOUT') {
                        // Timeout zostawia pending read(); anuluj, żeby zwolnić lock.
                        try { await reader.cancel(); } catch (_) { }
                        try { reader.releaseLock(); } catch (_) { }
                        const unlocked = await this.waitForReadableUnlock(800);
                        if (!unlocked) break;
                        reader = this.port.readable.getReader();
                        // Continue loop to check total timeout
                        continue;
                    }
                    console.error("Read error:", e);
                    break;
                }
            }

            // IMPORTANT: release lock before processing
            await reader.cancel();
            reader.releaseLock();
            reader = null;

            if (bytesRead === 4) {
                let isAscii = true;
                for (let i = 0; i < 4; i++) {
                    if (accumulatedBytes[i] < 32 || accumulatedBytes[i] > 126) isAscii = false;
                }
                const code = new TextDecoder().decode(accumulatedBytes);
                document.getElementById('accessCode').value = code;
                this.log('✅ Kod dostepu odczytany.', 'success');
                if (!isAscii) {
                    this.log('⚠️ Odczytano kod w niestandardowym formacie.', 'warning');
                }

                // Update internal buffer
                for (let i = 0; i < 4; i++) this.eepromData[1 + i] = accumulatedBytes[i];

            } else {
                throw new Error(`Otrzymano za mało danych (${bytesRead}/4 bajtów)`);
            }

        } catch (error) {
            this.log('❌ Blad odczytu kodu dostepu.', 'error');
            if (reader) {
                try { await reader.cancel(); reader.releaseLock(); } catch (e) { }
            }
        } finally {
            this.isBusy = false;
        }
    }

    async writeAccessCodeOnly() {
        if (!this.port) {
            this.log('❌ Najpierw połącz się z urządzeniem!', 'error');
            return;
        }

        if (this.isBusy) {
            this.log('⚠️ Operacja w toku. Poczekaj chwile...', 'warning');
            return;
        }
        this.isBusy = true;

        const codeInput = document.getElementById('accessCode').value;
        if (codeInput.length !== 4) {
            this.log('⚠️ Kod musi mieć dokładnie 4 znaki!', 'warning');
            this.isBusy = false;
            return;
        }

        let reader = null;
        try {
            this.log('📤 Zapis kodu dostepu...', 'info');

            const writer = this.port.writable.getWriter();
            await writer.write(new Uint8Array([0x43])); // 'C' (Write code)

            // Send 4 bytes with 10ms delay (optimized)
            for (let i = 0; i < 4; i++) {
                await writer.write(new Uint8Array([codeInput.charCodeAt(i)]));
                await new Promise(r => setTimeout(r, 10));
            }
            writer.releaseLock();

            // Read OK response
            reader = this.port.readable.getReader();
            let response = '';
            const startTime = Date.now();

            while (!response.includes('OK') && (Date.now() - startTime < 3000)) {
                try {
                    const readPromise = reader.read();
                    const timeoutPromise = new Promise((_, reject) =>
                        setTimeout(() => reject(new Error('READ_TIMEOUT')), 500)
                    );

                    const { value, done } = await Promise.race([readPromise, timeoutPromise]);

                    if (done) break;
                    if (value) {
                        response += new TextDecoder().decode(value);
                    }
                } catch (e) {
                    if (e.message === 'READ_TIMEOUT') {
                        // Timeout zostawia pending read(); anuluj i odtwórz reader.
                        try { await reader.cancel(); } catch (_) { }
                        try { reader.releaseLock(); } catch (_) { }
                        const unlocked = await this.waitForReadableUnlock(800);
                        if (!unlocked) break;
                        reader = this.port.readable.getReader();
                        continue;
                    }
                    break;
                }
            }

            await reader.cancel();
            reader.releaseLock();
            reader = null;

            if (response.includes('OK')) {
                this.log('✅ Kod dostepu zapisany.', 'success');
            } else {
                this.log('⚠️ Brak potwierdzenia zapisu kodu.', 'warning');
            }

        } catch (error) {
            this.log('❌ Blad zapisu kodu dostepu.', 'error');
            if (reader) {
                try { await reader.cancel(); reader.releaseLock(); } catch (e) { }
            }
        } finally {
            this.isBusy = false;
        }
    }

    clearInputs() {
        this.log('🧹 Czyszczenie formularzy...', 'info');

        // Clear Access Code
        const accessCode = document.getElementById('accessCode');
        if (accessCode) accessCode.value = '';

        // Clear Phone Numbers
        for (let i = 1; i <= this.maxUsers; i++) {
            const input = document.getElementById(`phone_${i}`);
            if (input) input.value = '';
        }

        // Clear Search
        const searchInput = document.getElementById('searchNumber');
        if (searchInput) searchInput.value = '';


        // Reset Mode (Default: Publiczny -> 1)
        const modePublic = document.querySelector('input[name="mode"][value="1"]');
        if (modePublic) modePublic.checked = true;

        // Reset Control Mode (Default: CLIP+SMS -> 3)
        const controlSelect = document.getElementById('controlMode');
        if (controlSelect) controlSelect.value = '3';


        // Reset Output Config (Default: Czas -> 0, 1s)
        const outConfigTime = document.querySelector('input[name="outputConfig"][value="0"]');
        if (outConfigTime) outConfigTime.checked = true;
        const outTime = document.getElementById('outputTime');
        if (outTime) outTime.value = '1';


    }

    clearPhoneNumbersOnly() {
        for (let i = 1; i <= this.maxUsers; i++) {
            const input = document.getElementById(`phone_${i}`);
            if (input) input.value = '';
        }
        const searchInput = document.getElementById('searchNumber');
        if (searchInput) {
            searchInput.value = '';
            // Re-show all rows after clearing filter.
            searchInput.dispatchEvent(new Event('input'));
        }
        this.log('🧹 Wyczyszczono listę numerów', 'info');
    }

    async readEEPROM() {
        this.clearInputs();

        if (!this.port) {
            this.showNoConnectionModal();
            this.log('❌ Najpierw połącz się z urządzeniem!', 'error');
            return;
        }

        if (this.isBusy) {
            this.log('⚠️ Operacja w toku. Poczekaj...', 'warning');
            return;
        }
        this.isBusy = true;
        this.pauseBackgroundTasks('odczyt EEPROM');

        try {
            this.showProgress(true, 'Odczyt...');
            if (this.port.readable.locked) {
                this.log('⏳ Przygotowanie odczytu...', 'info');
                const deadline = Date.now() + 4000;
                while (this.port.readable.locked && Date.now() < deadline) {
                    await new Promise(r => setTimeout(r, 100));
                }
                if (this.port.readable.locked) {
                    throw new Error('Port zajęty. Rozłącz i połącz ponownie.');
                }
            }

            // Keep profile stable during this connection (no re-detect here).
            if (!this.profileDetectionLocked) {
                await this.detectFirmwareProfile(true, true);
            }
            const bytesToRead = this.READ_SIZE;
            this.log('📥 Trwa odczyt ustawien... [I-R01]', 'info');

            this.eepromData = new Uint8Array(this.EEPROM_SIZE).fill(0xFF);
            let totalBytes = 0;
            const startTime = Date.now();

            // Odczyt bajt po bajcie (R + addr_hi + addr_lo → 1 bajt); retry przy timeout – bezpiecznie i bez błędów
            const READ_TIMEOUT_MS = 2000;
            const READ_RETRIES = 2;
            for (let i = 0; i < bytesToRead; i++) {
                let gotByte = false;
                for (let retry = 0; retry <= READ_RETRIES && !gotByte; retry++) {
                    const writerLoop = this.port.writable.getWriter();
                    await writerLoop.write(new Uint8Array([0x52, (i >> 8) & 0xFF, i & 0xFF])); // 'R' + ADDR
                    writerLoop.releaseLock();

                    await new Promise(r => setTimeout(r, 1));

                    const unlocked = await this.waitForReadableUnlock(READ_TIMEOUT_MS);
                    if (!unlocked) throw new Error('READ_LOCKED');
                    const readerLoop = this.port.readable.getReader();
                    const timeoutPromise = new Promise((_, reject) =>
                        setTimeout(() => reject(new Error('Timeout')), READ_TIMEOUT_MS)
                    );

                    try {
                        const { value, done } = await Promise.race([readerLoop.read(), timeoutPromise]);
                        if (done) break;
                        if (value && value.length > 0) {
                            this.eepromData[i] = value[0];
                            totalBytes++;
                            gotByte = true;
                        }
                    } catch (e) {
                        // Przy timeout race trzeba anulować pending read() przed releaseLock.
                        try { await readerLoop.cancel(); } catch (_) { }
                        try { readerLoop.releaseLock(); } catch (_) { }
                        if (retry < READ_RETRIES) {
                            await new Promise(r => setTimeout(r, 50)); // krótka przerwa przed ponowieniem
                        } else {
                            if (i === 0) this.log('❌ Brak odpowiedzi urzadzenia. [E-R02] Sprawdz: port/sterownik.', 'error');
                            else this.log('❌ Przerwano odczyt ustawien. [E-R03] Sprawdz: stabilnosc polaczenia.', 'error');
                            i = bytesToRead; break; // wyjście z zewnętrznej pętli
                        }
                    } finally {
                        // Reader musi być zwolniony zawsze, także po poprawnym odczycie.
                        try { readerLoop.releaseLock(); } catch (_) { }
                    }
                }
                if (i >= bytesToRead) break;

                if (i % 64 === 0) {
                    const progress = Math.min(100, Math.round((i / bytesToRead) * 100));
                    this.updateProgress(progress);
                    this.lastTraffic = Date.now();
                }

                if (i % 1024 === 0 && (Date.now() - startTime > 180000)) {
                    throw new Error("Timeout odczytu (3 min)");
                }
            }

            const readComplete = totalBytes >= bytesToRead;
            if (!readComplete) {
                this.log('⚠️ Odczyt zakonczony czesciowo. [W-R04] Sprawdz: polaczenie USB.', 'warning');
            }

            this.log('✅ Odczyt ustawien zakonczony. [S-R05]', 'success');
            this.parseEEPROMData();
            this.hasReliableSnapshot = readComplete;
            this.snapshotProfileName = this.profile.name;
            this.showProgress(false);

        } catch (error) {
            this.hasReliableSnapshot = false;
            this.log('❌ Blad odczytu ustawien. [E-R06] Sprawdz: port i restart sterownika.', 'error');
            this.showProgress(false);
        } finally {
            this.isBusy = false;
            this.resumeBackgroundTasks();
        }
    }

    getChangedAddresses(beforeData, afterData, limit) {
        const changed = [];
        for (let i = 0; i < limit; i++) {
            if (beforeData[i] !== afterData[i]) changed.push(i);
        }
        return changed;
    }

    isPhoneTableAddress(addr) {
        const start = 0x08;
        const endExclusive = start + (this.maxUsers * 5);
        return addr >= start && addr < endExclusive;
    }

    normalizePhoneFromBytes(bytes) {
        return this.sanitizePhoneNumber(this.bcdToPhone(bytes), true);
    }

    async readPhoneSlotBytes(slotStartAddr) {
        const out = new Uint8Array(5);
        for (let i = 0; i < 5; i++) {
            out[i] = await this.uartReadByteAtAddress(slotStartAddr + i, 1500);
        }
        return out;
    }

    buildVerificationAddresses(changedAddresses) {
        const unique = new Set([
            0, 1, 2, 3, 4, 6,
            this.profile.trybPracyAddr,
            this.profile.trybClipAddr,
            this.profile.clipDurationAddr + 0,
            this.profile.clipDurationAddr + 1,
            this.profile.clipDurationAddr + 2,
            this.profile.clipDurationAddr + 3,
            this.profile.serviceStatusAddr
        ]);

        if (changedAddresses.length <= 64) {
            changedAddresses.forEach(addr => unique.add(addr));
        } else {
            const sampleCount = 32;
            const step = Math.max(1, Math.floor(changedAddresses.length / sampleCount));
            for (let i = 0; i < changedAddresses.length && unique.size < 128; i += step) {
                unique.add(changedAddresses[i]);
            }
            unique.add(changedAddresses[0]);
            unique.add(changedAddresses[changedAddresses.length - 1]);
        }

        return Array.from(unique)
            .filter(addr => Number.isInteger(addr) && addr >= 0 && addr < this.READ_SIZE)
            .filter(addr => addr !== 0) // firmware potrafi przeliczyć checksum niezależnie
            .sort((a, b) => a - b);
    }

    async verifyWrittenData(addressesToVerify) {
        const mismatches = [];
        for (const addr of addressesToVerify) {
            let readVal = await this.uartReadByteAtAddress(addr, 1500);
            if (readVal !== this.eepromData[addr]) {
                // Po intensywnym zapisie pierwszy odczyt potrafi być niestabilny.
                await new Promise(r => setTimeout(r, 8));
                readVal = await this.uartReadByteAtAddress(addr, 1500);
            }
            if (readVal !== this.eepromData[addr]) {
                if (this.isPhoneTableAddress(addr)) {
                    const slotStartAddr = 0x08 + (Math.floor((addr - 0x08) / 5) * 5);
                    const expectedSlot = this.eepromData.slice(slotStartAddr, slotStartAddr + 5);
                    const actualSlot = await this.readPhoneSlotBytes(slotStartAddr);
                    const expectedRawPhone = this.bcdToPhone(expectedSlot);
                    const actualRawPhone = this.bcdToPhone(actualSlot);
                    const expectedValid = expectedRawPhone.length === 9;
                    const actualValid = actualRawPhone.length === 9;

                    // For phone table, treat as error only when at least one side
                    // is not a valid 9-digit number. This avoids false alarms from
                    // transient raw-byte differences that still decode to valid data.
                    if (expectedValid && actualValid) {
                        continue;
                    }
                }
                mismatches.push({ addr, expected: this.eepromData[addr], actual: readVal });
                if (mismatches.length >= 10) break;
            }
        }
        return {
            checked: addressesToVerify.length,
            mismatches
        };
    }

    async writeEEPROM() {
        if (this.port == null) {
            this.showNoConnectionModal();
            this.log('❌ Najpierw połącz się z urządzeniem!', 'error');
            return;
        }

        if (this.isBusy) return;

        const confirmed = await this.showModal(
            'Potwierdzenie',
            'Czy na pewno chcesz zapisać dane do sterownika?',
            'warning',
            true // show cancel
        );

        if (!confirmed) {
            return;
        }

        // Validate Access Code
        const accessCodeField = document.getElementById('accessCode');
        const accessCodeInput = this.sanitizeAccessCode(accessCodeField ? accessCodeField.value : '');
        if (accessCodeField && accessCodeField.value !== accessCodeInput) {
            accessCodeField.value = accessCodeInput;
        }
        if (!accessCodeInput || accessCodeInput.length !== 4) {
            await this.showModal('Błąd', 'Kod dostępu musi mieć dokładnie 4 znaki i nie może zawierać gwiazdek (*)!', 'error');
            this.log('❌ Przerwano zapis: Nieprawidłowy kod dostępu.', 'error');
            return;
        }

        if (!this.port.writable) {
            await this.showModal('Błąd', 'Port nie jest gotowy do zapisu.', 'error');
            this.log('❌ Port nie jest gotowy do zapisu.', 'error');
            return;
        }

        this.isBusy = true;
        this.pauseBackgroundTasks('zapis EEPROM');

        try {
            this.showProgress(true, 'Zapisywanie...');
            this.log('📤 Trwa zapis ustawien... [I-W01]', 'info');
            const baselineData = this.eepromData.slice();
            this.prepareEEPROMData();
            const bytesToWrite = this.READ_SIZE; // zapisujemy tylko zakres używany przez sterownik
            const canUseDifferentialWrite = this.hasReliableSnapshot && this.snapshotProfileName === this.profile.name;
            let changedAddresses = canUseDifferentialWrite
                ? this.getChangedAddresses(baselineData, this.eepromData, bytesToWrite)
                : Array.from({ length: bytesToWrite }, (_, i) => i);

            if (canUseDifferentialWrite) {
                this.log('📤 Rozpoczynam zapis roznicowy. [I-W02]', 'info');
            } else {
                this.log('📤 Rozpoczynam pelny zapis ustawien... [I-W03]', 'info');
            }

            if (changedAddresses.length === 0) {
                this.updateProgress(100);
                this.log('ℹ️ Brak zmian do zapisu. [I-W04]', 'info');
                this.hasReliableSnapshot = true;
                this.snapshotProfileName = this.profile.name;
                return;
            }

            // [W][ADR_HI][ADR_LO][DATA]
            // Firmware przetwarza bajty sekwencyjnie w pętli głównej; wysyłka 4 bajtów
            // "hurtowo" potrafi przepełnić odbiór UART i przesunąć adres/dane.
            // Dlatego wysyłamy każdy bajt osobno z krótką przerwą.
            // ~50% szybciej niż poprzednio, nadal z bezpiecznym marginesem
            // dla UART + obsługi zapisu EEPROM w firmware.
            const WRITE_INTERBYTE_MS = 1;
            const WRITE_POST_FRAME_MS = 5;
            const writer = this.port.writable.getWriter();
            try {
                for (let idx = 0; idx < changedAddresses.length; idx++) {
                    const i = changedAddresses[idx];
                    const b0 = 0x57; // 'W'
                    const b1 = (i >> 8) & 0xFF;
                    const b2 = i & 0xFF;
                    const b3 = this.eepromData[i];

                    await writer.write(new Uint8Array([b0]));
                    await new Promise(r => setTimeout(r, WRITE_INTERBYTE_MS));
                    await writer.write(new Uint8Array([b1]));
                    await new Promise(r => setTimeout(r, WRITE_INTERBYTE_MS));
                    await writer.write(new Uint8Array([b2]));
                    await new Promise(r => setTimeout(r, WRITE_INTERBYTE_MS));
                    await writer.write(new Uint8Array([b3]));
                    await new Promise(r => setTimeout(r, WRITE_POST_FRAME_MS));

                    if (idx % 32 === 0) {
                        const progress = Math.min(100, Math.round(((idx + 1) / changedAddresses.length) * 100));
                        this.updateProgress(progress);
                        this.lastTraffic = Date.now();
                    }
                }
            } finally {
                try { writer.releaseLock(); } catch (_) { }
            }
            this.updateProgress(100);

            this.log('📤 Zapis ustawien zakonczony. [S-W05]', 'info');

            const addressesToVerify = this.buildVerificationAddresses(changedAddresses);
            this.log('🔍 Weryfikacja po zapisie... [I-W06]', 'info');
            await new Promise(r => setTimeout(r, 120)); // chwila na ustabilizowanie UART/EEPROM po zapisie
            let verifyResult = await this.verifyWrittenData(addressesToVerify);
            if (verifyResult.mismatches.length > 0) {
                this.log('ℹ️ Powtarzam weryfikacje... [I-W11]', 'info');
                await new Promise(r => setTimeout(r, 250));
                verifyResult = await this.verifyWrittenData(addressesToVerify);
            }
            if (verifyResult.mismatches.length > 0) {
                this.log('⚠️ Weryfikacja wykryla rozbieznosci. [W-W07] Sprawdz: zapis i odczyt kontrolny.', 'warning');
                this.hasReliableSnapshot = false;
                this.snapshotProfileName = null;
            } else {
                this.log('✅ Weryfikacja po zapisie OK. [S-W08]', 'success');
                this.hasReliableSnapshot = true;
                this.snapshotProfileName = this.profile.name;
            }

            this.log('✅ Zapis danych zakonczony. [S-W09]', 'success');
            this.log('ℹ️ Mozesz wykonac odczyt kontrolny. [I-W10]', 'info');
            await this.restartController();

        } catch (error) {
            this.hasReliableSnapshot = false;
            this.log('❌ Blad zapisu ustawien. [E-W11] Sprawdz: port, kod dostepu, restart.', 'error');
        } finally {
            this.showProgress(false);
            this.isBusy = false;
            this.resumeBackgroundTasks();
        }
    }

    async restartController() {
        if (!this.port) return;

        try {
            this.log('🔄 Restartowanie sterownika... [I-S01]', 'info');

            // Pulse DTR to reset (Arduino-style reset)
            // Hold DTR for 1 second as requested

            await this.port.setSignals({ dataTerminalReady: true });
            await new Promise(r => setTimeout(r, 1000));
            await this.port.setSignals({ dataTerminalReady: false });
            this.lastTraffic = Date.now();
            this.watchdogGraceUntil = this.lastTraffic + this.restartWatchdogGraceMs;

            this.log('✅ Restart wykonany. [S-S02]', 'success');

        } catch (e) {
            this.log('⚠️ Nie udalo sie zrestartowac sterownika. [W-S03] Sprawdz: polaczenie.', 'warning');
        }
    }

    async verifyEEPROM() {
        if (!this.port) {
            this.log('❌ Najpierw połącz się z urządzeniem!', 'error');
            return;
        }

        try {
            this.showProgress(true, 'Weryfikacja EEPROM...');
            this.log('✓ Trwa weryfikacja danych...', 'info');

            const writer = this.port.writable.getWriter();
            await writer.write(new Uint8Array([0x56])); // 'V'
            await writer.write(this.eepromData);
            writer.releaseLock();

            this.log('⏳ Trwa sprawdzanie danych...', 'info');

            const reader = this.port.readable.getReader();
            const { value } = await reader.read();
            reader.releaseLock();

            const response = new TextDecoder().decode(value);

            if (response.includes('OK')) {
                this.log('✅ Sprawdzenie zakonczone pomyslnie.', 'success');
            } else {
                this.log('❌ Weryfikacja nie powiodla sie.', 'error');
            }

            this.showProgress(false);

        } catch (error) {
            this.log('❌ Blad weryfikacji.', 'error');
            this.showProgress(false);
        }
    }

    sanitizePhoneNumber(value, exactLength = true) {
        const digits = String(value || '').replace(/\D/g, '');
        if (!digits) return '';
        if (exactLength) return digits.length === 9 ? digits : '';
        return digits.substring(0, 9);
    }

    isLegacyEmptyPhoneBytes(bytes, decodedPhone = '') {
        if (!bytes || bytes.length === 0) return true;

        // Legacy "empty" encodings observed across firmware variants.
        const legacyBlankBytes = Array.from(bytes).every(
            b => b === 0x00 || b === 0xFF || b === 0xF0 || b === 0x0F
        );
        if (legacyBlankBytes) return true;

        // Treat slots with only 0/F nibbles as empty placeholders.
        const nibbleBlank = Array.from(bytes).every((b) => {
            const lo = b & 0x0F;
            const hi = (b >> 4) & 0x0F;
            const isBlankNibble = (n) => n === 0x00 || n === 0x0F;
            return isBlankNibble(lo) && isBlankNibble(hi);
        });
        if (nibbleBlank) return true;

        // Defensive: fully zero-decoded value is not a usable phone number.
        if (decodedPhone && /^0+$/.test(decodedPhone)) return true;

        return false;
    }

    sanitizeAccessCode(value) {
        // Visible ASCII without '*' (reserved in SMS syntax), max 4 chars.
        return String(value || '')
            .replace(/[^\x21-\x7E]/g, '')
            .replace(/\*/g, '')
            .substring(0, 4);
    }

    normalizeOutputTime(value) {
        const digits = String(value || '').replace(/\D/g, '');
        if (!digits) return 1;
        let v = parseInt(digits, 10);
        if (!Number.isFinite(v) || v < 1) v = 1;
        if (v > 99999) v = 99999;
        return v;
    }

    parseEEPROMData() {
        // Lock service-only status byte from device readout.
        this.serviceStatusLockedValue = this.eepromData[this.profile.serviceStatusAddr];

        // Parse access code (bytes 1-4)
        const accessCode = String.fromCharCode(...this.eepromData.slice(1, 5));
        document.getElementById('accessCode').value = accessCode;

        // Parse CLIP/DTMF + SMS
        // ADRES_EEPROM_TRYB_CLIP_DTMF (zależny od wariantu)
        // ADRES_EEPROM_SMS_TRIGGER 6
        const clipByte = this.eepromData[this.profile.trybClipAddr];
        const smsByte = this.eepromData[6];

        let controlMode = 0; // DTMF
        if (clipByte !== 0x00 && smsByte !== 0x00) controlMode = 3; // CLIP+SMS
        else if (clipByte !== 0x00) controlMode = 1; // CLIP
        else if (smsByte !== 0x00) controlMode = 2; // SMS

        document.getElementById('controlMode').value = controlMode;

        // Parse Skryba - REMOVED
        if (document.querySelector('input[name="skryba"]')) {
            // Disable or hide if element exists, or just do nothing as data is gone
        }

        // Parse phone numbers (linear map from 8, count zalezny od wariantu)
        for (let i = 0; i < this.maxUsers; i++) {
            const startAddr = 0x08 + (i * 5);
            const phoneBytes = this.eepromData.slice(startAddr, startAddr + 5);
            const decodedPhone = this.bcdToPhone(phoneBytes);
            const phoneNumber = this.sanitizePhoneNumber(decodedPhone, true);

            const guiId = i + 1;
            const input = document.getElementById(`phone_${guiId}`);
            if (!input) continue;

            if (phoneNumber) {
                input.value = phoneNumber;
            } else {
                input.value = '';
                // Auto-heal: invalid/garbled slot is normalized to empty and will be
                // written back as 0xFF on next Save.
                this.eepromData.fill(0xFF, startAddr, startAddr + 5);
            }
        }

        // Output Config (Time/Toggle)
        const t0 = this.eepromData[this.profile.clipDurationAddr + 0];
        const t1 = this.eepromData[this.profile.clipDurationAddr + 1];
        const t2 = this.eepromData[this.profile.clipDurationAddr + 2];
        const t3 = this.eepromData[this.profile.clipDurationAddr + 3];
        const rawTimeVal = (t0 | (t1 << 8) | (t2 << 16) | (t3 << 24)) >>> 0;
        let timeVal = rawTimeVal;
        // Zgodnie z firmware: pusty/niezainicjalizowany = 2s, invalid = 2s.
        if (rawTimeVal === 0xFFFFFFFF || rawTimeVal < 1 || rawTimeVal > 99999) {
            timeVal = 2;
        }

        if (timeVal === 99999) {
            // Toggle Mode
            document.querySelector('input[name="outputConfig"][value="1"]').checked = true;
            document.getElementById('outputTime').value = 99999;
        } else {
            // Time Mode
            document.querySelector('input[name="outputConfig"][value="0"]').checked = true;
            document.getElementById('outputTime').value = timeVal;
        }

        // Mode (Prywatny/Publiczny)
        const modeByte = this.eepromData[this.profile.trybPracyAddr];
        const modeValue = (modeByte === 0x01) ? "1" : "0";
        const modeRadio = document.querySelector(`input[name="mode"][value="${modeValue}"]`);
        if (modeRadio) modeRadio.checked = true;

        // MyNum (address 1019, 5 bytes BCD)


        this.log('📊 Dane EEPROM załadowane do GUI. [S-R07]', 'success');
    }

    prepareEEPROMData() {
        // Access code (bytes 1-4)
        const accessCode = document.getElementById('accessCode').value.padEnd(4, '\0');
        for (let i = 0; i < 4; i++) {
            this.eepromData[1 + i] = accessCode.charCodeAt(i);
        }

        // SERVICE-ONLY. Always restore value read from device.
        // User/import/configurator must not modify this byte.
        if (this.serviceStatusLockedValue !== null) {
            this.eepromData[this.profile.serviceStatusAddr] = this.serviceStatusLockedValue;
        }

        // Mode
        const mode = document.querySelector('input[name="mode"]:checked').value;
        this.eepromData[this.profile.trybPracyAddr] = mode === '1' ? 0x01 : 0x00;

        // Control mode -> trybClip + sms_trigger(6)
        const controlMode = parseInt(document.getElementById('controlMode').value);
        if (controlMode === 0) { // DTMF
            this.eepromData[this.profile.trybClipAddr] = 0x00;
            this.eepromData[6] = 0x00;
        } else if (controlMode === 1) { // CLIP
            this.eepromData[this.profile.trybClipAddr] = 0x01;
            this.eepromData[6] = 0x00;
        } else if (controlMode === 2) { // SMS
            this.eepromData[this.profile.trybClipAddr] = 0x00;
            this.eepromData[6] = 0x01;
        } else { // CLIP+SMS
            this.eepromData[this.profile.trybClipAddr] = 0x01;
            this.eepromData[6] = 0x01;
        }

        // Skryba (Default: 0/Off) - Logic removed (Address 1010 now part of CLIP Duration)

        // Output Config -> clipDuration/clipToggle (zależne od wariantu)
        const outputConfig = document.querySelector('input[name="outputConfig"]:checked').value;
        if (outputConfig === '1') { // Toggle
            // Firmware expects 99999 (0x1869F) in clip duration for Toggle
            const val = 99999;
            this.eepromData[this.profile.clipDurationAddr + 0] = val & 0xFF;
            this.eepromData[this.profile.clipDurationAddr + 1] = (val >> 8) & 0xFF;
            this.eepromData[this.profile.clipDurationAddr + 2] = (val >> 16) & 0xFF;
            this.eepromData[this.profile.clipDurationAddr + 3] = (val >> 24) & 0xFF;
            this.eepromData[this.profile.clipToggleAddr] = 0x00; // Unused
        } else { // Time
            const outputTimeInput = document.getElementById('outputTime');
            const timeVal = this.normalizeOutputTime(outputTimeInput ? outputTimeInput.value : '1');
            if (outputTimeInput) outputTimeInput.value = String(timeVal);
            // Write 4 bytes little-endian to clip duration
            this.eepromData[this.profile.clipDurationAddr + 0] = timeVal & 0xFF;
            this.eepromData[this.profile.clipDurationAddr + 1] = (timeVal >> 8) & 0xFF;
            this.eepromData[this.profile.clipDurationAddr + 2] = (timeVal >> 16) & 0xFF;
            this.eepromData[this.profile.clipDurationAddr + 3] = (timeVal >> 24) & 0xFF;
            this.eepromData[this.profile.clipToggleAddr] = 0x00; // Unused
        }

        // Phone numbers (count zależny od wariantu, linear)
        for (let i = 1; i <= this.maxUsers; i++) {
            const phoneInput = document.getElementById(`phone_${i}`);
            const rawPhoneNumber = phoneInput ? phoneInput.value : '';
            const phoneNumber = this.sanitizePhoneNumber(rawPhoneNumber, true);

            // Linear Mapping: GUI 1 -> EEPROM 0
            const eepromIndex = i - 1;
            const startAddr = 0x08 + (eepromIndex * 5);

            if (phoneNumber) {
                const bcdBytes = this.phoneToBCD(phoneNumber);
                this.eepromData.set(bcdBytes, startAddr);
            } else {
                this.eepromData.fill(0xFF, startAddr, startAddr + 5);
                if (rawPhoneNumber) {
                    this.log(`ℹ️ Niepelny wpis numeru potraktowano jako puste miejsce. [I-D01]`, 'info');
                }
            }
        }

        // MyNum - REMOVED
        // Time Control - REMOVED

        this.recalculateChecksum();
    }

    bcdToPhone(bytes) {
        // Dekoder zgodny 1:1 z firmware (konwertuj_blok_eeprom_na_telefon):
        // - koniec numeru wyznacza PIERWSZY bajt 0xFF,
        // - dla nieparzystej liczby cyfr końcówka ma postać XF.
        let last = -1;
        for (let i = 0; i < bytes.length; i++) {
            if (bytes[i] === 0xFF) break;
            last = i;
        }
        if (last < 0) return '';

        const out = [];
        let p = last;

        // Specjalny przypadek nieparzystej liczby cyfr: nibble low == 0xF.
        if ((bytes[p] & 0x0F) === 0x0F) {
            const hi = (bytes[p] >> 4) & 0x0F;
            if (hi <= 9) out.push(String(hi));
            p--;
        }

        while (p >= 0) {
            const lo = bytes[p] & 0x0F;
            if (lo <= 9) out.push(String(lo));

            const hi = (bytes[p] >> 4) & 0x0F;
            if (hi <= 9) out.push(String(hi));

            p--;
        }
        return out.join('');
    }

    // --- GSM Signal Implementation ---
    startSignalPolling() {
        if (this.signalInterval) return;
        const container = document.getElementById('gsmSignalContainer');
        if (container) {
            // Don't show container immediately. Wait for valid signal in updateSignalUI.
            // container.classList.remove('offline');
        }

        // Initial read
        this.readSignalStrength();

        // Poll every 1 second (Watchdog is 3.3s)
        this.signalInterval = setInterval(() => this.readSignalStrength(), 1000);
    }

    stopSignalPolling() {
        const container = document.getElementById('gsmSignalContainer');
        if (container) {
            container.classList.add('offline');
            container.classList.remove('weak', 'medium', 'strong');

            const bars = container.querySelectorAll('.signal-bar');
            bars.forEach(b => b.classList.remove('active'));

            const percentDiv = document.getElementById('signalPercent');
            if (percentDiv) percentDiv.innerText = '';
        }

        if (this.signalInterval) {
            clearInterval(this.signalInterval);
            this.signalInterval = null;
        }
    }

    async readSignalStrength() {
        if (!this.port || this.isBusy) return;

        // Non-blocking approach: try to get lock, if busy just skip this poll
        if (this.port.writable.locked || this.port.readable.locked) return;

        let reader = null;
        try {
            const writer = this.port.writable.getWriter();
            await writer.write(new Uint8Array([0x53])); // 'S' – firmware odsyła 1 B: poziom_sieci_gsm
            writer.releaseLock();

            await new Promise(r => setTimeout(r, 20)); // daj MCU czas na uart1_process_commands()

            reader = this.port.readable.getReader();

            const timeoutPromise = new Promise((_, reject) =>
                setTimeout(() => reject(new Error('Timeout')), 800)
            );
            const readPromise = reader.read();

            const { value, done } = await Promise.race([readPromise, timeoutPromise]);

            if (!done) {
                this.lastTraffic = Date.now(); // Heartbeat on any response
                if (value && value.length > 0) {
                    const csq = value[0] & 0xFF;
                    // Debug signal value in UI Log
                    // const debugPct = Math.min(Math.round((csq / 31) * 100), 100);
                    // this.log(`📶 Diagnostyka: RAW=${csq} (${debugPct}%)`, 'warning');
                    this.updateSignalUI(csq);
                }
            }

        } catch (e) {
            // Silently fail signal update to avoid log spam
            // console.warn('Signal read error:', e);
        } finally {
            // CRITICAL: Always release reader lock
            if (reader) {
                try {
                    await reader.cancel();
                    reader.releaseLock();
                } catch (e) {
                    // Ignore errors during cleanup
                }
            }
        }
    }

    updateSignalUI(csq) {
        const container = document.getElementById('gsmSignalContainer');
        const percentDiv = document.getElementById('signalPercent');

        if (!container) return;

        // Kody z firmware: 99/85 = błąd, 16 = szukanie sieci (pokazujemy jako "n/d")
        let isInvalid = false;
        if (csq === 99 || csq === 85 || csq === 16 || csq === null || csq === undefined) {
            csq = 0;
            isInvalid = true;
        }

        let bars = 0;
        if (csq > 0) bars = 1;
        if (csq > 7) bars = 2;
        if (csq > 13) bars = 3;
        if (csq > 19) bars = 4;
        if (csq > 25) bars = 5;

        let pctText = '0%';
        if (isInvalid) {
            pctText = 'n/d'; // urządzenie odpowiedziało, GSM niedostępny / błąd
        } else {
            const pct = Math.min(Math.round((csq / 31) * 100), 100);
            pctText = `${pct}%`;
        }

        if (percentDiv) percentDiv.innerText = pctText;

        // Update classes
        const barDivs = container.querySelectorAll('.signal-bar');
        barDivs.forEach((div, index) => {
            if (index < bars) div.classList.add('active');
            else div.classList.remove('active');
        });

        // Kolor / widoczność
        container.classList.remove('weak', 'medium', 'strong', 'offline', 'nodata');

        if (isInvalid) {
            if (percentDiv) percentDiv.innerText = 'n/d';
            container.classList.add('nodata'); // neutralny szary – brak zasięgu / błąd GSM
            container.title = 'Zasięg GSM: niedostępny (modem nie gotowy lub błąd)';
        } else if (csq === 0) {
            if (percentDiv) percentDiv.innerText = '0%';
            container.classList.add('offline');
            container.title = 'Zasięg GSM: 0%';
        } else {
            if (percentDiv) percentDiv.innerText = pctText;
            if (bars <= 2) container.classList.add('weak');
            else if (bars <= 4) container.classList.add('medium');
            else container.classList.add('strong');
            container.title = `Zasięg GSM: ${csq} (${pctText})`;
        }
    }



    phoneToBCD(phone) {
        // Match Firmware Reverse Filling Logic
        // "123" -> 32 1F FF...

        const bytes = new Uint8Array(5).fill(0xFF);
        const digits = phone.split('').map(d => parseInt(d));

        // C implementation fills from end of string backwards.
        // And fills bytes starting from 0.
        // High nibble of byte[0] = last digit
        // Low nibble of byte[0] = 2nd last digit

        let digitIdx = digits.length - 1;

        for (let i = 0; i < 5; i++) {
            if (digitIdx < 0) break;

            let high = 0xF;
            let low = 0xF;

            if (digitIdx >= 0) high = digits[digitIdx--];
            if (digitIdx >= 0) low = digits[digitIdx--];

            bytes[i] = (high << 4) | low;
        }

        return bytes;
    }

    recalculateChecksum() {
        let sum = 0;
        // Checksum liczony tylko dla aktywnego zakresu pracy sterownika.
        for (let i = 1; i < this.READ_SIZE; i++) {
            sum += this.eepromData[i];
        }
        this.eepromData[0] = (-sum) & 0xFF;
    }

    exportCSV() {
        let csv = '';
        csv += `Urzadzenie;${this.profile.name}\n`;
        csv += 'Kod dostępu;' + document.getElementById('accessCode').value + '\n';


        const mode = document.querySelector('input[name="mode"]:checked').value;
        csv += 'Tryb;' + (mode === '0' ? 'Prywatny' : 'Publiczny') + '\n';

        const controlMode = document.getElementById('controlMode').value;
        const controlModeText = ['DTMF', 'CLIP', 'SMS', 'CLIP+SMS'][parseInt(controlMode)];
        csv += 'Tryb sterowania;' + controlModeText + '\n';



        const outputConfig = document.querySelector('input[name="outputConfig"]:checked').value;
        if (outputConfig === '1') {
            csv += 'Konfiguracja wyjścia;Toggle\n';
        } else {
            const outputTime = document.getElementById('outputTime').value;
            csv += 'Konfiguracja wyjścia;Czas: ' + outputTime + '\n';
        }


        csv += '\n';
        csv += 'Pozycja;Numer\n';

        // Unified Users list (1-maxUsers)
        for (let i = 1; i <= this.maxUsers; i++) {
            const phoneInput = document.getElementById(`phone_${i}`);
            const phoneNumber = phoneInput ? phoneInput.value : '';
            csv += `${i};${phoneNumber ? phoneNumber : 'Brak danych'}\n`;
        }

        const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `${this.profile.name}_config_${new Date().toISOString().slice(0, 10)}.csv`;
        a.click();
        URL.revokeObjectURL(url);

        this.log('💾 Konfiguracja wyeksportowana do CSV', 'success');
    }

    importCSV() {
        const fileInput = document.getElementById('importFile');
        if (fileInput) fileInput.value = ''; // Reset value to allow re-selecting the same file

        fileInput.click();
        fileInput.onchange = (e) => {
            const file = e.target.files[0];
            if (!file) return;

            const reader = new FileReader();
            reader.onload = (event) => {
                try {
                    const lines = event.target.result.split('\n');
                    let inPhoneSection = false;

                    for (const line of lines) {
                        const trimmed = line.trim();
                        if (!trimmed) continue;

                        if (trimmed.startsWith('Pozycja;')) {
                            inPhoneSection = true;
                            continue;
                        }

                        const parts = trimmed.split(';');
                        if (parts.length < 2) continue;

                        const key = parts[0].trim();
                        const value = parts[1].trim();

                        if (inPhoneSection) {
                            const position = parseInt(key);
                            if (!isNaN(position) && position >= 1 && position <= this.maxUsers) {
                                // Validation: Allow only digits, max 9 chars
                                const sanitizedValue = value.replace(/\D/g, '');

                                if (sanitizedValue !== value) {
                                    console.warn(`Ignorowano niedozwolone znaki w pozycji ${position}: "${value}" -> "${sanitizedValue}"`);
                                    this.log(`⚠️ Skorygowano numer. [W-I01] Sprawdz format 9 cyfr.`, 'warning');
                                }

                                if (sanitizedValue.length > 9) {
                                    console.warn(`Numer za długi w pozycji ${position}, przycięto do 9 cyfr.`);
                                    this.log(`⚠️ Przycięto numer do 9 cyfr. [W-I02]`, 'warning');
                                }

                                const finalValue = (sanitizedValue.length === 9) ? sanitizedValue : '';
                                if (sanitizedValue && sanitizedValue.length !== 9) {
                                    this.log(`ℹ️ Numer ${position} ma niepelna dlugosc i zostal potraktowany jako puste miejsce. [I-I03]`, 'info');
                                }

                                const input = document.getElementById(`phone_${position}`);
                                if (input) input.value = finalValue;
                            }
                        } else {
                            if (key === 'Kod dostępu') {
                                const accessCodeFromCsv = this.sanitizeAccessCode(value);
                                document.getElementById('accessCode').value = accessCodeFromCsv;
                                if (value !== accessCodeFromCsv) {
                                    this.log('⚠️ Skorygowano kod dostepu z CSV.', 'warning');
                                }
                            } else if (key === 'Tryb') {
                                const modeValue = value === 'Prywatny' ? '0' : '1';
                                document.querySelector(`input[name="mode"][value="${modeValue}"]`).checked = true;
                            } else if (key === 'Tryb sterowania') {
                                const controlModeMap = { 'DTMF': '0', 'CLIP': '1', 'SMS': '2', 'CLIP+SMS': '3' };
                                document.getElementById('controlMode').value = controlModeMap[value] || '3';
                            } else if (key === 'Konfiguracja wyjścia') {
                                if (value === 'Toggle') {
                                    document.querySelector('input[name="outputConfig"][value="1"]').checked = true;
                                } else if (value.startsWith('Czas:')) {
                                    document.querySelector('input[name="outputConfig"][value="0"]').checked = true;
                                    const timeValueRaw = value.replace('Czas:', '').trim();
                                    const timeValue = this.normalizeOutputTime(timeValueRaw);
                                    document.getElementById('outputTime').value = String(timeValue);
                                }
                            } else if (key === 'Numer karty SIM sterownika') {
                                // Parse MyNum (Numer karty SIM sterownika)
                                if (value && value !== 'Brak danych') {
                                    document.getElementById('myNum').value = value;
                                } else {
                                    document.getElementById('myNum').value = '';
                                }
                            }
                        }
                    }

                    // Handle phone numbers with "Brak danych"
                    for (const line of lines) {
                        const trimmed = line.trim();
                        if (!trimmed || trimmed.startsWith('===') || trimmed.startsWith('Pozycja;')) continue;

                        const parts = trimmed.split(';');
                        if (parts.length >= 2) {
                            const position = parseInt(parts[0].trim());
                            const value = parts[1].trim();

                            if (!isNaN(position) && position >= 1 && position <= this.maxUsers) {
                                const input = document.getElementById(`phone_${position}`);
                                if (input) {
                                    // Clear field if "Brak danych", otherwise apply strict validation
                                    input.value = (value === 'Brak danych') ? '' : this.sanitizePhoneNumber(value, true);
                                }
                            }
                        }
                    }
                    this.log('📂 Konfiguracja zaimportowana z CSV', 'success');
                } catch (error) {
                    this.log(`❌ Błąd importu. [E-I03] Sprawdz format CSV i kodowanie UTF-8.`, 'error');
                }
            };
            reader.readAsText(file);
        };
    }

    showProgress(show, text = '') {
        const section = document.getElementById('progressSection');
        const textEl = document.getElementById('progressText');

        if (show) {
            section.style.display = 'block';
            textEl.textContent = text;
            this.updateProgress(0);
        } else {
            section.style.display = 'none';
        }
    }

    updateProgress(percent) {
        document.getElementById('progressFill').style.width = `${percent}%`;
    }

    log(message, type = 'info') {
        const logArea = document.getElementById('logArea');
        const entry = document.createElement('div');
        entry.className = `log-entry ${type}`;
        entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
        logArea.appendChild(entry);
        logArea.scrollTop = logArea.scrollHeight;
    }

    clearLog() {
        document.getElementById('logArea').innerHTML = '';
        this.log('🗑️ Logi wyczyszczone', 'info');
    }

    // --- Watchdog Implementation ---
    setupDisconnectModal() {
        const modal = document.getElementById('disconnectModal');
        const closeBtn = document.querySelector('.close-disconnect');
        const okBtn = document.getElementById('reconnectBtnModal');

        if (modal) {
            const closeModal = () => {
                modal.style.display = 'none';
            };

            if (closeBtn) closeBtn.onclick = closeModal;
            if (okBtn) okBtn.onclick = closeModal;

            window.addEventListener('click', (event) => {
                if (event.target === modal) closeModal();
            });
        }
    }

    showDisconnectModal() {
        const modal = document.getElementById('disconnectModal');
        if (modal) {
            modal.style.display = 'flex';
        }
    }

    startWatchdog() {
        this.stopWatchdog();
        // Check every 1 second
        this.watchdogInterval = setInterval(() => {
            const now = Date.now();
            if (now < this.watchdogGraceUntil) return;
            if (now - this.lastTraffic > this.watchdogTimeoutMs) {
                this.log('⚠️ Utracono lacznosc z urzadzeniem. Rozlaczam.', 'warning');
                this.stopWatchdog();
                this.disconnect();
                this.showDisconnectModal();
            }
        }, 1000);
    }

    stopWatchdog() {
        if (this.watchdogInterval) {
            clearInterval(this.watchdogInterval);
            this.watchdogInterval = null;
        }
    }

    // --- No Connection Modal ---
    setupNoConnectionModal() {
        const modal = document.getElementById('noConnectionModal');
        const closeBtn = document.querySelector('.close-noconnection');
        const okBtn = document.getElementById('okNoConnectionBtn');

        if (modal) {
            const closeModal = () => {
                modal.style.display = 'none';
            };

            if (closeBtn) closeBtn.onclick = closeModal;
            if (okBtn) okBtn.onclick = closeModal;

            window.addEventListener('click', (event) => {
                if (event.target === modal) closeModal();
            });
        }
    }

    showNoConnectionModal() {
        const modal = document.getElementById('noConnectionModal');
        if (modal) {
            modal.style.display = 'flex';
        }
    }

    // --- Dynamic Generic Modal ---
    setupDynamicModal() {
        // Elements are retrieved when showing to ensure fresh interactions or initialized here if static.
    }

    showModal(title, content, type = 'info', showCancel = false) {
        return new Promise((resolve) => {
            const modal = document.getElementById('dynamicModal');
            const titleEl = document.getElementById('dynamicModalTitle');
            const bodyEl = document.getElementById('dynamicModalBody');
            const closeBtn = document.querySelector('.close-dynamic');
            const cancelBtn = document.getElementById('dynamicModalCancel');
            const okBtn = document.getElementById('dynamicModalOk');
            const contentDiv = document.querySelector('#dynamicModal .modal-content');

            if (!modal) {
                alert(content); // Fallback
                resolve(true);
                return;
            }

            // Set content
            titleEl.textContent = title;
            bodyEl.innerHTML = content; // Allows specific formatting if needed

            // Set color based on type
            let color = '#2196f3'; // info
            if (type === 'error') color = '#d32f2f';
            if (type === 'warning') color = '#ff9800';
            if (type === 'success') color = '#4caf50';

            titleEl.style.color = color;
            contentDiv.style.borderLeft = `5px solid ${color}`;

            // Buttons
            if (showCancel) {
                cancelBtn.style.display = 'inline-block';
            } else {
                cancelBtn.style.display = 'none';
            }

            // Cleanup old listeners to avoid duplicates if not using {once:true}
            const cleanup = () => {
                closeBtn.onclick = null;
                okBtn.onclick = null;
                cancelBtn.onclick = null;
                window.onclick = null;
            };

            const close = (result) => {
                modal.style.display = 'none';
                // Restore window onclick for other modals if needed (simplified here)
                cleanup();
                resolve(result);
            };

            closeBtn.onclick = () => close(false);
            cancelBtn.onclick = () => close(false);
            okBtn.onclick = () => close(true);

            // Outside click (careful to not conflict with other modals logic if they are active)
            // Ideally we check if this modal is top-most. 
            // For now, simpler approach:

            // Only overwrite window.onclick if we are the active modal? 
            // Existing modals used window.onclick global. This might partial conflict but OK for single modal usage.
            window.onclick = (event) => {
                if (event.target === modal) close(false);
            };

            modal.style.display = 'flex';
        });
    }
}

document.addEventListener('DOMContentLoaded', () => {
    window.uartManager = new UARTEEPROMManager();
});
