/**********************************************************************************************************************************

**********************************************************************************************************************************/
// const Properties                    = PropertiesService.getScriptProperties();

// const WEB_PIN                       = Properties.getProperty("WEB_PIN");

// const DEVICE_KEY                    = Properties.getProperty("DEVICE_KEY");

// const COMMAND                       = Properties.getProperty("COMMAND");

// const STATUS                        = Properties.getProperty("STATUS");

// const WAIT_TIME                     = Properties.getProperty("WAIT_TIME");

// const Lock                          = LockService.getScriptLock();

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function ProjectSetConfig() 
{
    const Properties = PropertiesService.getScriptProperties();

    Properties.setProperty("WEB_PIN", "1234");

    Properties.setProperty("DEVICE_KEY", "ESP8266WOL920PLUS_A7F93C21");

    Properties.setProperty("COMMAND", "IDLE");

    Properties.setProperty("STATUS", "IDLE");
    
    Properties.setProperty("WAIT_TIME", "150");

    Properties.setProperty("NAS_IP", "192.168.1.200");

    Properties.setProperty("NAS_MAC", "10:90:28:EA:2F:DC");
}

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function GetConfig()
{
    const Properties = PropertiesService.getScriptProperties();

    return {
        NAS_IP: Properties.getProperty("NAS_IP") || "192.168.1.200",

        NAS_MAC: Properties.getProperty("NAS_MAC") || "11:22:33:44:55:66"
    };
}

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function SaveConfig(Data)
{
    const Properties = PropertiesService.getScriptProperties();

    if (!Data || Data.PIN !== Properties.getProperty("WEB_PIN"))
    {
        return { STATUS: false, MESSAGE: "Mã PIN không đúng!" };
    }

    if (Data.NAS_IP) Properties.setProperty("NAS_IP", Data.NAS_IP.trim());

    if (Data.NAS_MAC) Properties.setProperty("NAS_MAC", Data.NAS_MAC.trim());

    return { STATUS: true, MESSAGE: "Đã cập nhật IP và MAC thành công!" };
}

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function GetStatus() 
{
    const Properties = PropertiesService.getScriptProperties();

    return {
        STATUS: Properties.getProperty("STATUS") || "IDLE",

        WAIT_TIME: parseInt(Properties.getProperty("WAIT_TIME") || "150", 10),

        COMMAND: Properties.getProperty("COMMAND") || "IDLE"
    };
}

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function ProjectPrint() 
{   
    const Properties = PropertiesService.getScriptProperties();

    console.log("WEB_PIN:", Properties.getProperty("WEB_PIN"));

    console.log("DEVICE_KEY:", Properties.getProperty("DEVICE_KEY"));

    console.log("COMMAND:", Properties.getProperty("COMMAND"));
}





/**********************************************************************************************************************************

**********************************************************************************************************************************/
function doGet(Data) 
{
    console.log("[1] Bat dau doGet");

    const Properties = PropertiesService.getScriptProperties();

    const Action = Data && Data.parameter ? Data.parameter.ACTION : "";

    console.log("[2] ACTION =", Action);
    
    if(Action === "POLL" || Action === "ACK") 
    {
        const KEY = Data.parameter.KEY || "";

        if(!KEY || KEY !== Properties.getProperty("DEVICE_KEY")) 
        {
            return JSONOutPut({ STATUS: false, ERROR: "Unauthorized" });
        }

        if(Action === "POLL") 
        {
            console.log("[3] Dang xu ly POLL");

            //  Đọc giá trị của thuộc tính có tên "COMMAND" từ bộ lưu trữ của hệ thống (Properties).
            //  Nếu không tìm thấy hoặc giá trị bị trống (null/undefined), biến Command sẽ mặc định nhận giá trị là "IDLE" (chờ/rảnh rỗi).
            const Command = Properties.getProperty("COMMAND") || "IDLE";

            //  STATUS: true: Xử lý yêu cầu thành công.
            //  CMD: Kiểm tra biến Command. Nếu Command đang lưu là "WOL_PENDING" (đang chờ gửi lệnh bật máy), 
            //  server sẽ trả về chữ "WOL". Ngược lại, trả về chữ "NONE" (không có lệnh gì cả).
            return JSONOutPut({STATUS: true, CMD: Command === "WOL_PENDING" ? "WOL" : "NONE"});
        }

        if (Action === "ACK") 
        {
            //  Khóa đồng bộ (Script Lock): Yêu cầu script dừng chờ tối đa 5 giây (5000 ms) để chiếm quyền truy cập duy nhất.
            //  Mục đích: Tránh hiện tượng tranh chấp dữ liệu (Race Condition) khi có nhiều yêu cầu gửi lên server cùng một lúc.
            const lock = LockService.getScriptLock();

            lock.waitLock(5000);

            try 
            {
                //  Lấy trạng thái hiện tại do thiết bị gửi lên qua tham số STATUS (nếu không có sẽ mặc định là "IDLE").
                const Status = Data.parameter.STATUS || Data.parameter.status || "IDLE";

                //  Lấy thời gian chờ do thiết bị gửi lên qua tham số WAIT_TIME (mặc định "150").
                const WaitTime = Data.parameter.WAIT_TIME || Data.parameter.waitTime || "150";

                //  Lưu trạng thái mới của thiết bị vào bộ nhớ hệ thống.
                Properties.setProperty("STATUS", Status);

                //  Lưu thời gian chờ mới vào bộ nhớ hệ thống.
                Properties.setProperty("WAIT_TIME", WaitTime);

                //  Nếu thiết bị báo về trạng thái đã thành công ("ONLINE") hoặc thất bại ("FAILED"), 
                //  lệnh lưu trong hệ thống sẽ được xóa bằng cách reset lại: Properties.setProperty("COMMAND", "IDLE");.
                //  Việc này giúp ở lần POLL tiếp theo, server sẽ không tiếp tục gửi lại lệnh WOL nữa.
                if (Status === "ONLINE" || Status === "FAILED") 
                {
                    Properties.setProperty("COMMAND", "IDLE");
                }
            } 
            finally 
            {
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

/******************************************************************************************************************************

******************************************************************************************************************************/
function RequestWake(Pin)
{
    const Properties = PropertiesService.getScriptProperties();
    
    //  Xác thực bảo mật: Kiểm tra xem người dùng có truyền mã Pin lên hay không, 
    //  và mã đó có trùng khớp với mã PIN lưu trong hệ thống (WEB_PIN) hay không.
    //  Nếu sai hoặc thiếu PIN: Trả về lỗi ngay lập tức { STATUS: false, MESSAGE: "PIN không đúng." }.
    if (!Pin || Pin !== Properties.getProperty("WEB_PIN"))
    {
        return { STATUS: false, MESSAGE: "PIN không đúng." };
    }

    const lock = LockService.getScriptLock();

    try
    {
        //  Khóa chống xung đột: Thử chiếm quyền truy cập script trong vòng 5 giây.
        //  Nếu quá 5 giây mà có thao tác khác đang giữ khóa (!hasLock), hàm dừng lại và báo về: "Hệ thống đang bận, vui lòng thử lại sau.".
        const hasLock = lock.tryLock(5000);

        if (!hasLock) 
        {
            return { STATUS: false, MESSAGE: "Hệ thống đang bận, vui lòng thử lại sau." };
        }

        //  Đặt biến COMMAND thành "WOL_PENDING". 
        //  Đây chính là tín hiệu để ở hàm POLL trước đó, server biết và trả về lệnh "WOL" cho thiết bị ESP.
        Properties.setProperty("COMMAND", "WOL_PENDING");

        //  Cập nhật trạng thái hệ thống thành "WAITING_ESP" (Đang chờ thiết bị ESP nhận và xử lý).
        Properties.setProperty("STATUS", "WAITING_ESP");

        return { STATUS: true, MESSAGE: "Đã gửi yêu cầu. Đang chờ ESP xử lý..." };
    }
    finally
    {
        lock.releaseLock();
    }
}





/**********************************************************************************************************************************

**********************************************************************************************************************************/
function Include(filename) 
{
    return HtmlService.createHtmlOutputFromFile(filename).getContent();
}

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function JSONOutPut(data) 
{
    return ContentService
        .createTextOutput(JSON.stringify(data))
        .setMimeType(ContentService.MimeType.JSON);
}