const PROP = PropertiesService.getScriptProperties();

function setupConfig() {
  PROP.setProperty("WEB_PIN", "1234");
  PROP.setProperty("DEVICE_KEY", "ESP8266WOL920PLUS_A7F93C21");
  PROP.setProperty("COMMAND", "IDLE");
  PROP.setProperty("STATUS", "IDLE");
  PROP.setProperty("WAIT_TIME", "150");
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
        const status = e.parameter.status || "IDLE";
        const waitTime = e.parameter.waitTime || "150";

        PROP.setProperty("STATUS", status);
        PROP.setProperty("WAIT_TIME", waitTime);

        if (status === "ONLINE" || status === "FAILED") {
          PROP.setProperty("COMMAND", "IDLE");
        }
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
    return { ok: false, message: "PIN không đúng." };
  }

  const lock = LockService.getScriptLock();

  lock.waitLock(5000);

  try {
    PROP.setProperty("COMMAND", "WOL_PENDING");

    PROP.setProperty("STATUS", "WAITING_ESP");

    return {
      ok: true,
      message: "Đã gửi yêu cầu. Đang chờ ESP xử lý..."
    };
  } finally {
    lock.releaseLock();
  }
}

function getSystemStatus() {
  return {
    ok: true,
    command: PROP.getProperty("COMMAND") || "IDLE",
    status: PROP.getProperty("STATUS") || "IDLE",
    waitTime: parseInt(PROP.getProperty("WAIT_TIME") || "150", 10)
  };
}

function jsonOutput(data) {
  return ContentService
    .createTextOutput(JSON.stringify(data))
    .setMimeType(ContentService.MimeType.JSON);
}