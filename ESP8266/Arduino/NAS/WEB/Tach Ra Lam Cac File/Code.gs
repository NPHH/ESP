
// function testDoGet() 
// {
//     const mockEvent = 
//     {
//         parameter: 
//         {
//             action: "poll",
//             key: "MÃ_KEY_CỦA_BẠN",
//             debug: "true"
//         }
//     };
    
//     // In trực tiếp danh sách thông số truyền vào ra ô Log
//     console.log("Tham số truyền vào (e.parameter):", mockEvent.parameter);

//     // Gọi hàm doGet(e) để chạy logic và in kết quả parameter trả về
//     const result = doGet(mockEvent);
// }



// function testDoGet() 
// {
//     // 1. Gia lap tham so ESP8266 gui len Web App
//     const mockEvent = 
//     {
//         parameter: 
//         {
//             ACTION: "POLL",
//             KEY: "12344321",
//             DEBUG: "true"
//         }
//     };

//     console.log("===== TEST doGet =====");

//     console.log("Tham so gui len:");

//     console.log(JSON.stringify(mockEvent.parameter, null, 2));

//     console.log(mockEvent);


//     const result = doGet(mockEvent);

//     // 3. Kiem tra doi tuong phan hoi
//     console.log("Doi tuong phan hoi:");

//     console.log(result);

  
//     if (result && typeof result.getContent === "function")
//     {
//         const responseBody = result.getContent();

//         console.log("===== RESPONSE BODY =====");
//         console.log(responseBody);

//         // Thu phan tich JSON neu phan hoi la JSON
//         try
//         {
//             const json = JSON.parse(responseBody);

//             console.log("===== JSON PARSED =====");
//             console.log(JSON.stringify(json, null, 2));
//         }
//         catch (error)
//         {
//             console.log("Phan hoi khong phai JSON hop le.");
//         }
//     }
//     else
//     {
//         console.log("Khong doc duoc noi dung phan hoi.");
//         console.log("Hay kiem tra gia tri return cua doGet().");
//     }

//     console.log("===== KET THUC TEST =====");
// }
















function doGet(e) {
  const action = e && e.parameter ? e.parameter.action : "";

  if (action === "poll" || action === "ack") 
  {
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

  // Render Template để ghép các file HTML lại với nhau
  return HtmlService.createTemplateFromFile("Index")
    .evaluate()
    .setTitle("Wake On LAN NAS")
    .addMetaTag('viewport', 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no');
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