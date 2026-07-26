const STATUS_INFO = {
    healthy: { label: "正常に稼働中", cssClass: "healthy" },
    unhealthy: { label: "異常あり", cssClass: "unhealthy" },
    down: { label: "停止中", cssClass: "down" },
    unknown: { label: "不明", cssClass: "unknown" },
};

function renderCards(results) {
    const grid = document.getElementById("grid");
    grid.innerHTML = "";

    for (const result of results) {
        const info = STATUS_INFO[result.Status] || STATUS_INFO.unknown;

        const card = document.createElement("div");
        card.className = `card ${info.cssClass}`;

        const details = (result.Details || [])
            .map(d => `<p class="detail">${d.Label}: ${d.Value}</p>`)
            .join("");

        card.innerHTML = `
            <h2>${result.Name}</h2>
            <p>${info.label}</p>
            ${details}
        `;

        grid.appendChild(card);
    }
}

function pollStatus() {
    fetch("/api/status")
        .then(res => res.json())
        .then(renderCards);
}

pollStatus();
setInterval(pollStatus, 5000);
