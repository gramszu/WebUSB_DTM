
const initIntro = () => {
    const introScreen = document.getElementById('introScreen');
    const mainAppContainer = document.getElementById('mainAppContainer');
    const launchBtn = document.getElementById('launchBtn');

    const logLines = document.querySelectorAll('.boot-logs .log-line');

    if (mainAppContainer) mainAppContainer.style.display = 'none';

    if (launchBtn) {
        launchBtn.disabled = false;
        launchBtn.classList.add('ready');

        launchBtn.addEventListener('click', () => {
            setTimeout(() => {
                window.location.href = 'app/index.html';
            }, 1000);
        });
    }

    logLines.forEach((line, index) => {
        setTimeout(() => {
            line.style.opacity = '1';
            line.style.transform = 'translateY(0)';
        }, 300 + (index * 400));
    });

    setupOfflineLinks();
    setupDynamicModal();

    function setupOfflineLinks() {
        document.querySelectorAll('.offline-aware').forEach(link => {
            link.addEventListener('click', async (e) => {
                e.preventDefault();
                const targetUrl = link.href;
                let isOffline = !navigator.onLine;

                if (!isOffline) {
                    try {
                        const controller = new AbortController();
                        const timeoutId = setTimeout(() => controller.abort(), 1500);
                        await fetch('https://www.google.com/generate_204?' + Date.now(), {
                            method: 'HEAD',
                            mode: 'no-cors',
                            cache: 'no-store',
                            signal: controller.signal
                        });
                        clearTimeout(timeoutId);
                    } catch (error) {
                        isOffline = true;
                    }
                }

                if (isOffline) {
                    showModal('Brak Internetu', 'Wymagany jest dostęp do internetu, aby otworzyć ten zasób!', 'warning');
                } else {
                    window.open(targetUrl, '_blank');
                }
            });
        });
    }

    function showModal(title, message, type = 'info', showCancel = false) {
        return new Promise((resolve) => {
            const modal = document.getElementById('dynamicModal');
            const titleEl = document.getElementById('dynamicModalTitle');
            const bodyEl = document.getElementById('dynamicModalBody');
            const okBtn = document.getElementById('dynamicModalOk');
            const cancelBtn = document.getElementById('dynamicModalCancel');
            const closeBtn = document.querySelector('.close-dynamic');

            if (!modal) {
                alert(message);
                resolve(true);
                return;
            }

            titleEl.textContent = title;
            bodyEl.innerHTML = message.replace(/\n/g, '<br>');
            titleEl.className = '';
            if (type === 'error') titleEl.style.color = '#f44336';
            else if (type === 'warning') titleEl.style.color = '#ff9800';
            else if (type === 'success') titleEl.style.color = '#4CAF50';
            else titleEl.style.color = 'inherit';

            if (showCancel) {
                cancelBtn.style.display = 'inline-block';
                cancelBtn.onclick = () => { modal.style.display = 'none'; resolve(false); };
            } else {
                cancelBtn.style.display = 'none';
            }

            const close = () => { modal.style.display = 'none'; resolve(true); };
            okBtn.onclick = close;
            if (closeBtn) closeBtn.onclick = () => { modal.style.display = 'none'; resolve(false); };
            window.onclick = (event) => {
                if (event.target === modal) { modal.style.display = 'none'; resolve(false); }
            };
            modal.style.display = 'flex';
        });
    }

    function setupDynamicModal() {}
};

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initIntro);
} else {
    initIntro();
}
