// ESP32 OTA Firmware Update Script
let uploadInProgress = false;

document.addEventListener("DOMContentLoaded", () => {
    document.querySelectorAll('.container').forEach(el => el.classList.add('fade-in'));

    const dropArea = document.getElementById("drop-area");
    const fileInput = document.getElementById("file_input");
    const progressBar = document.getElementById("progress_bar");
    const progressContainer = document.getElementById("progress_container");
    const statusText = document.getElementById("status");
    const btn_firmware = document.getElementById("githubFirmware");
    const btn_spiffs = document.getElementById("githubSPIFFS");
    const versionInfo = document.getElementById("versionInfo");

    fileInput.addEventListener('click', () => fileInput.value = '');
    dropArea.addEventListener("click", () => fileInput.click());

    dropArea.addEventListener("dragover", (e) => {
        e.preventDefault();
        dropArea.classList.add("dragover");
    });

    dropArea.addEventListener("dragleave", () => dropArea.classList.remove("dragover"));

    dropArea.addEventListener("drop", (e) => {
        e.preventDefault();
        dropArea.classList.remove("dragover");
        fileInput.value = "";
        handleUpload(e.dataTransfer.files[0]);
    });

    fileInput.addEventListener("change", () => handleUpload(fileInput.files[0]));

    btn_firmware.addEventListener("click", () => {
        fetchAndUploadFromGitHub(OTA_CONFIG.GITHUB_BASE_URL + OTA_CONFIG.FIRMWARE_BIN, "firmware.bin");
    });

    btn_spiffs.addEventListener("click", () => {
        fetchAndUploadFromGitHub(OTA_CONFIG.GITHUB_BASE_URL + OTA_CONFIG.SPIFFS_BIN, "spiffs.bin");
    });

    function handleUpload(file) {
        if (uploadInProgress) return;

        progressContainer.style.display = "block";
        progressBar.style.width = "0%";
        progressBar.classList.remove("error");
        statusText.textContent = "";
        uploadInProgress = true;

        if (!file || !file.name.endsWith(".bin")) {
            statusText.textContent = "Only .BIN files are allowed.";
            uploadInProgress = false;
            fileInput.value = '';
            return;
        }

        if (!["firmware.bin", "spiffs.bin"].includes(file.name)) {
            statusText.innerHTML = `${file.name} is not valid. Must be firmware.bin or spiffs.bin`;
            uploadInProgress = false;
            fileInput.value = '';
            return;
        }

        const isSPIFFS = file.name.toLowerCase().includes("spiffs");
        statusText.textContent = `Uploading ${isSPIFFS ? "SPIFFS" : "Firmware"}...`;

        const xhr = new XMLHttpRequest();
        xhr.open("POST", "/update");

        xhr.upload.onprogress = (e) => {
            progressBar.style.width = Math.round((e.loaded / e.total) * 100) + "%";
        };

        xhr.onload = () => {
            uploadInProgress = false;
            progressBar.classList.remove("uploading");
            fileInput.value = '';

            if (xhr.status === 200 && xhr.responseText === "OK") {
                progressBar.style.width = "100%";
                showModal("success", "Update Successful", "Your device is now rebooting.");
            } else {
                statusText.textContent = "OTA update failed. Please try again.";
                progressBar.style.width = "0%";
                progressBar.classList.add("error");
            }
        };

        xhr.onerror = () => {
            uploadInProgress = false;
            statusText.textContent = "Upload failed (network error)";
            progressBar.style.width = "0%";
            progressBar.classList.add("error");
            fileInput.value = '';
        };

        const form = new FormData();
        form.append("update", file, file.name);
        xhr.send(form);
    }

    function fetchAndUploadFromGitHub(url, name) {
        statusText.textContent = `Fetching ${name} from GitHub...`;
        progressBar.style.width = "0%";
        progressContainer.style.display = "block";

        fetch(url)
            .then(response => {
                if (!response.ok) throw new Error("Failed to fetch " + name);
                return response.blob();
            })
            .then(blob => {
                handleUpload(new File([blob], name, { type: "application/octet-stream" }));
            })
            .catch(err => {
                statusText.textContent = err.message;
                fileInput.value = '';
            });
    }

    function showModal(type, title, message) {
        const backdrop = document.createElement("div");
        backdrop.classList.add("modal-backdrop");
        document.body.append(backdrop);

        const modal = document.createElement("div");
        modal.classList.add("modal-box", type);
        modal.innerHTML = `<h3>${title}</h3><p>${message}</p><button>OK</button>`;

        backdrop.append(modal);
        backdrop.style.display = "flex";

        modal.querySelector("button").addEventListener("click", () => {
            backdrop.remove();
            if (type === "success") window.location.reload(true);
        });
    }

    // Fetch current device versions and compare with GitHub
    fetch("/get_config")
        .then(res => res.json())
        .then(device => {
            const currentFirmware = device.version_firmware || "0.0";
            const currentSPIFFS = device.version_spiffs || "0.0";
            versionInfo.textContent = `Current Firmware: v${currentFirmware}, SPIFFS: v${currentSPIFFS}`;

            fetch(OTA_CONFIG.GITHUB_BASE_URL + OTA_CONFIG.VERSION_JSON)
                .then(res => res.json())
                .then(github => {
                    const githubFirmware = github.firmware || "0.0";
                    const githubSPIFFS = github.spiffs || "0.0";
                    const firmwareNeedsUpdate = currentFirmware !== githubFirmware;
                    const spiffsNeedsUpdate = currentSPIFFS !== githubSPIFFS;

                    if (firmwareNeedsUpdate) {
                        btn_firmware.classList.add("update-available");
                        btn_firmware.disabled = false;
                        btn_firmware.textContent = `Update Firmware to v${githubFirmware}`;
                    } else {
                        btn_firmware.disabled = true;
                        btn_firmware.textContent = `Firmware is Up-to-Date`;
                    }

                    if (spiffsNeedsUpdate) {
                        if (firmwareNeedsUpdate) {
                            btn_spiffs.disabled = true;
                            btn_spiffs.textContent = `Update Firmware first`;
                            btn_spiffs.classList.add("waiting");
                        } else {
                            btn_spiffs.classList.add("update-available");
                            btn_spiffs.disabled = false;
                            btn_spiffs.textContent = `Update SPIFFS to v${githubSPIFFS}`;
                        }
                    } else {
                        btn_spiffs.disabled = true;
                        btn_spiffs.textContent = `SPIFFS is Up-to-Date`;
                    }
                })
                .catch(() => {
                    btn_firmware.textContent = "Firmware check failed";
                    btn_spiffs.textContent = "SPIFFS check failed";
                    btn_firmware.classList.add("error");
                    btn_spiffs.classList.add("error");
                    btn_firmware.disabled = true;
                    btn_spiffs.disabled = true;
                });
        }).catch(() => {
            btn_firmware.textContent = "Device config failed";
            btn_spiffs.textContent = "Device config failed";
            btn_firmware.disabled = true;
            btn_spiffs.disabled = true;
        });
});
