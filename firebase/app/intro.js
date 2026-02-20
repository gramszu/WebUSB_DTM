
document.addEventListener('DOMContentLoaded', () => {
    const introScreen = document.getElementById('introScreen');
    const mainAppContainer = document.getElementById('mainAppContainer');
    const launchBtn = document.getElementById('launchBtn');
    const loaderFill = document.getElementById('introLoaderFill');
    const loaderPercent = document.getElementById('loaderPercent');
    const logLines = document.querySelectorAll('.boot-logs .log-line');

    // Disable main app container initially (just in case)
    if (mainAppContainer) mainAppContainer.style.display = 'none';

    // Animation 1: Progress Bar & Logs
    let progress = 0;
    const totalDuration = 2500; // 2.5 seconds total load time
    const intervalTime = 30; // Update every 30ms
    const increment = 100 / (totalDuration / intervalTime);

    const progressInterval = setInterval(() => {
        progress += increment;
        if (progress >= 100) {
            progress = 100;
            clearInterval(progressInterval);

            // Enable Launch Button
            launchBtn.disabled = false;
            launchBtn.classList.add('ready'); // Add a class for pulsing/distinct style if needed
        }

        loaderFill.style.width = `${progress}%`;
        loaderPercent.textContent = `${Math.floor(progress)}%`;

    }, intervalTime);

    // Logs sequencing
    logLines.forEach((line, index) => {
        setTimeout(() => {
            line.style.opacity = '1';
            line.style.transform = 'translateY(0)';
        }, 300 + (index * 400)); // Staggered delay
    });

    // Launch Button Handler
    launchBtn.addEventListener('click', () => {
        // Fade out intro
        introScreen.style.transition = 'opacity 0.5s ease, transform 0.5s ease';
        introScreen.style.opacity = '0';
        introScreen.style.transform = 'scale(0.95)';

        setTimeout(() => {
            introScreen.style.display = 'none';
            // Show main app
            mainAppContainer.style.display = 'block';
            mainAppContainer.style.opacity = '0';
            mainAppContainer.style.transition = 'opacity 0.5s ease';

            // Trigger reflow
            void mainAppContainer.offsetWidth;

            mainAppContainer.style.opacity = '1';

        }, 500);
    });
});
