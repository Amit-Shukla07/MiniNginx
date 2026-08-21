// Real-time polling of MiniNginx /api/stats
async function updateMetrics() {
    try {
        const res = await fetch('/api/stats');
        if (!res.ok) return;
        const data = await res.json();

        document.getElementById('val-uptime').textContent = `${data.uptime_seconds}s`;
        document.getElementById('val-active-conn').textContent = data.active_connections;
        document.getElementById('val-requests').textContent = data.total_requests;
        document.getElementById('val-2xx').textContent = data.responses_2xx;
        document.getElementById('val-4xx').textContent = data.responses_4xx;

        const totalBytes = data.bytes_read + data.bytes_written;
        let formattedBytes = `${(totalBytes / 1024).toFixed(1)} KB`;
        if (totalBytes > 1024 * 1024) {
            formattedBytes = `${(totalBytes / (1024 * 1024)).toFixed(2)} MB`;
        }
        document.getElementById('val-bytes').textContent = formattedBytes;
    } catch (e) {
        // Server might be temporarily restarting or offline
    }
}

// Interactive HTTP test console
document.getElementById('send-request-btn').addEventListener('click', async () => {
    const method = document.getElementById('request-method').value;
    const path = document.getElementById('request-path').value;
    const statusTag = document.getElementById('response-status');
    const bodyBox = document.getElementById('response-body');

    statusTag.textContent = 'SENDING...';
    statusTag.style.background = 'rgba(245, 158, 11, 0.2)';
    statusTag.style.color = '#f59e0b';

    try {
        const start = performance.now();
        const res = await fetch(path, { method });
        const latency = (performance.now() - start).toFixed(1);

        statusTag.textContent = `${res.status} ${res.statusText} (${latency}ms)`;
        if (res.ok) {
            statusTag.style.background = 'rgba(16, 185, 129, 0.2)';
            statusTag.style.color = '#10b981';
        } else {
            statusTag.style.background = 'rgba(239, 68, 68, 0.2)';
            statusTag.style.color = '#ef4444';
        }

        if (method === 'HEAD') {
            let headersText = '';
            for (let [k, v] of res.headers.entries()) {
                headersText += `${k}: ${v}\n`;
            }
            bodyBox.textContent = `[HEAD Response Headers]\n${headersText}`;
        } else {
            const text = await res.text();
            try {
                const jsonObj = JSON.parse(text);
                bodyBox.textContent = JSON.stringify(jsonObj, null, 2);
            } catch {
                bodyBox.textContent = text;
            }
        }
        updateMetrics();
    } catch (err) {
        statusTag.textContent = 'ERROR';
        statusTag.style.background = 'rgba(239, 68, 68, 0.2)';
        statusTag.style.color = '#ef4444';
        bodyBox.textContent = `Request failed: ${err.message}`;
    }
});

// Start polling loop
updateMetrics();
setInterval(updateMetrics, 1000);
