
const PROP = PropertiesService.getScriptProperties();

function setupConfig() {
  // Chay ham nay mot lan trong Apps Script.
  // Thay 2 chuoi ben duoi bang gia tri rieng cua ban.
  PROP.setProperty("WEB_PIN", "1234");
  PROP.setProperty("DEVICE_KEY", "ESP8266WOL920PLUS_A7F93C21");
  PROP.setProperty("COMMAND", "IDLE");
}

function doGet(e) {
  const action = e && e.parameter ? e.parameter.action : "";

  if (action === "poll" || action === "ack") {
    const key = e.parameter.key || "";

    if (!key || key !== PROP.getProperty("DEVICE_KEY")) {
      return jsonOutput({ ok: false, error: "Unauthorized" });
    }

    if (action === "poll") {
      const command = PROP.getProperty("COMMAND") || "IDLE";
      return jsonOutput({
        ok: true,
        command: command === "WOL_PENDING" ? "WOL" : "NONE"
      });
    }

    if (action === "ack") {
      const lock = LockService.getScriptLock();
      lock.waitLock(5000);
      try {
        PROP.setProperty("COMMAND", "IDLE");
      } finally {
        lock.releaseLock();
      }
      return jsonOutput({ ok: true });
    }
  }

  return HtmlService.createHtmlOutputFromFile("Index")
    .setTitle("Wake On LAN NAS");
}

function requestWake(pin) {
  if (!pin || pin !== PROP.getProperty("WEB_PIN")) {
    return { ok: false, message: "PIN khong dung." };
  }

  const lock = LockService.getScriptLock();
  lock.waitLock(5000);

  try {
    const current = PROP.getProperty("COMMAND") || "IDLE";

    if (current === "WOL_PENDING") {
      return {
        ok: true,
        message: "Lenh bat NAS dang cho ESP xu ly."
      };
    }

    PROP.setProperty("COMMAND", "WOL_PENDING");

    return {
      ok: true,
      message: "Da gui yeu cau. Cho ESP nhan lenh."
    };
  } finally {
    lock.releaseLock();
  }
}

function jsonOutput(data) {
  return ContentService
    .createTextOutput(JSON.stringify(data))
    .setMimeType(ContentService.MimeType.JSON);
}
