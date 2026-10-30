const Properties = PropertiesService.getScriptProperties();

function ProjectSetConfig() {
  const Properties = PropertiesService.getScriptProperties();
  Properties.setProperty("WEB_PIN", "1234");
  Properties.setProperty("DEVICE_KEY", "ESP8266WOL920PLUS_A7F93C21");
  Properties.setProperty("COMMAND", "IDLE");
  Properties.setProperty("STATUS", "IDLE");
  Properties.setProperty("WAIT_TIME", "150");
}

function GetConfig() {
  const Properties = PropertiesService.getScriptProperties();
  return {
    NAS_IP: Properties.getProperty("NAS_IP") || "192.168.1.200",
    NAS_MAC: Properties.getProperty("NAS_MAC") || "11:22:33:44:55:66"
  };
}

function GetStatus() {
  const Properties = PropertiesService.getScriptProperties();
  return {
    STATUS: Properties.getProperty("STATUS") || "IDLE",
    WAIT_TIME: parseInt(Properties.getProperty("WAIT_TIME") || "150", 10),
    COMMAND: Properties.getProperty("COMMAND") || "IDLE"
  };
}

function JSONOutPut(data) {
  return ContentService
    .createTextOutput(JSON.stringify(data))
    .setMimeType(ContentService.MimeType.JSON);
}

function doGet(data) {
  console.log("[1] Bat dau doGet");
  
  if (!data || !data.parameters) {
    return HtmlService.createTemplateFromFile('Index')
      .evaluate()
      .setTitle('NAS Wake On LAN')
      .addMetaTag('viewport', 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no')
      .setXFrameOptionsMode(HtmlService.XFrameOptionsMode.ALLOWALL);
  }

  const Action = data.parameters.ACTION || "";
  console.log("[2] ACTION =", Action);
  
  if (Action === "POLL" || Action === "ACK") {
    const KEY = data.parameters.KEY || "";
    if (!KEY || KEY !== Properties.getProperty("DEVICE_KEY")) {
      return JSONOutPut({ STATUS: false, ERROR: "Unauthorized" });
    }
    if (Action === "POLL") {
      console.log("[3] Dang xu ly POLL");
      const Command = Properties.getProperty("COMMAND") || "IDLE";
      return JSONOutPut({ STATUS: true, CMD: Command === "WOL_PENDING" ? "WOL" : "NONE" });
    }
    if (Action === "ACK") {
      const lock = LockService.getScriptLock();
      lock.waitLock(5000);
      try {
        const Status = data.parameters.STATUS || data.parameters.status || "IDLE";
        const WaitTime = data.parameters.WAIT_TIME || data.parameters.waitTime || "150";
        Properties.setProperty("STATUS", Status);
        Properties.setProperty("WAIT_TIME", WaitTime);
        if (Status === "ONLINE" || Status === "FAILED") {
          Properties.setProperty("COMMAND", "IDLE");
        }
      } finally {
        lock.releaseLock();
      }
      return JSONOutPut({ STATUS: true });
    }
  }

  return HtmlService.createTemplateFromFile('Index')
    .evaluate()
    .setTitle('NAS Wake On LAN')
    .addMetaTag('viewport', 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no')
    .setXFrameOptionsMode(HtmlService.XFrameOptionsMode.ALLOWALL);
}

function RequestWake(Pin) {
  const Properties = PropertiesService.getScriptProperties();
  if (!Pin || Pin !== Properties.getProperty("WEB_PIN")) {
    return { STATUS: false, MESSAGE: "PIN không đúng." };
  }
  const lock = LockService.getScriptLock();
  try {
    const hasLock = lock.tryLock(5000);
    if (!hasLock) {
      return { STATUS: false, MESSAGE: "Hệ thống đang bận, vui lòng thử lại sau." };
    }
    Properties.setProperty("COMMAND", "WOL_PENDING");
    Properties.setProperty("STATUS", "WAITING_ESP");
    return { STATUS: true, MESSAGE: "Đã gửi yêu cầu. Đang chờ ESP xử lý..." };
  } finally {
    lock.releaseLock();
  }
}

function Include(filename) {
  return HtmlService.createHtmlOutputFromFile(filename).getContent();
}

