/**********************************************************************************************************************************

**********************************************************************************************************************************/
const Properties                    = PropertiesService.getScriptProperties();

const WEB_PIN                       = Properties.getProperty("WEB_PIN");

const DEVICE_KEY                    = Properties.getProperty("DEVICE_KEY");

const COMMAND                       = Properties.getProperty("COMMAND");

const STATUS                        = Properties.getProperty("STATUS");

const WAIT_TIME                     = Properties.getProperty("WAIT_TIME");

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function ProjectSetConfig() 
{
    Properties.setProperty("WEB_PIN", "1234");

    Properties.setProperty("DEVICE_KEY", "ESP8266WOL920PLUS_A7F93C21");

    Properties.setProperty("COMMAND", "IDLE");

    Properties.setProperty("STATUS", "IDLE");
    
    Properties.setProperty("WAIT_TIME", "150");
}

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function GetSystemStatus() 
{
    return {
        ok: true,

        command: Properties.getProperty("COMMAND") || "IDLE",

        status: Properties.getProperty("STATUS") || "IDLE",

        waitTime: parseInt(Properties.getProperty("WAIT_TIME") || "150", 10)

        
    };
}

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function ProjectPrint() 
{   
    console.log("WEB_PIN:", WEB_PIN);

    console.log("DEVICE_KEY:", DEVICE_KEY);

    console.log("COMMAND:", COMMAND);
}





/**********************************************************************************************************************************

**********************************************************************************************************************************/
function doGet(Data) 
{
    console.log("[1] Bat dau doGet");

    const Action = Data && Data.parameter ? Data.parameter.ACTION : "";

    console.log("[2] ACTION =", Action);
    
    if(Action === "POLL" || Action === "ACK") 
    {
        const KEY = Data.parameter.KEY || "";

        if(!KEY || KEY !== Properties.getProperty("DEVICE_KEY")) 
        {
            return JSONOutput({ STATUS: false, ERROR: "Unauthorized" });
        }

        if(Action === "POLL") 
        {
            console.log("[3] Dang xu ly POLL");

            const Command = Properties.getProperty("COMMAND") || "IDLE";

            return JSONOutput({STATUS: true, CMD: Command === "WOL_PENDING" ? "WOL" : "NONE"});
        }

        if (Action === "ACK") 
        {
            const lock = LockService.getScriptLock();

            lock.waitLock(5000);

            try 
            {
                const status = Data.parameter.status || "IDLE";

                const waitTime = Data.parameter.waitTime || "150";

                Properties.setProperty("STATUS", status);

                Properties.setProperty("WAIT_TIME", waitTime);

                if (status === "ONLINE" || status === "FAILED") 
                {
                    Properties.setProperty("COMMAND", "IDLE");
                }
            } 
            finally 
            {
                lock.releaseLock();
            }

            return JSONOutput({ STATUS: true });
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
    if (!Pin || Pin !== Properties.getProperty("WEB_PIN"))
    {
        return { STATUS: false, MESSAGE: "PIN không đúng." };
    }

    try
    {
        const hasLock = lock.tryLock(5000);

        if (!hasLock) 
        {
            return { STATUS: false, MESSAGE: "Hệ thống đang bận, vui lòng thử lại sau." };
        }

        Properties.setProperty("COMMAND", "WOL_PENDING");

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
function JSONOutput(data) 
{
    return ContentService
        .createTextOutput(JSON.stringify(data))
        .setMimeType(ContentService.MimeType.JSON);
}


