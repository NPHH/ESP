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
function ProjectPrint() 
{   
    console.log("WEB_PIN:", WEB_PIN);

    console.log("DEVICE_KEY:", DEVICE_KEY);

    console.log("COMMAND:", COMMAND);
}




function testDoGet() 
{
    const mockEvent = 
    {
        parameter: 
        {
            action: "poll",
            key: "MÃ_KEY_CỦA_BẠN",
            debug: "true"
        }
    };
    
    // In trực tiếp danh sách thông số truyền vào ra ô Log
    console.log("Tham số truyền vào (e.parameter):", mockEvent.parameter);

    // Gọi hàm doGet(e) để chạy logic và in kết quả parameter trả về
    const result = doGet(mockEvent);
}









/**********************************************************************************************************************************

**********************************************************************************************************************************/
function doGet(e) 
{
    console.log("doGet nhận được e.parameter:", e?.parameter);
    
    if (e && e.parameter && e.parameter.debug === "true") 
    {
       
    }

    

    return HtmlService.createTemplateFromFile('Index')
            .evaluate()
            .setTitle('NAS Wake On LAN')
            .addMetaTag('viewport', 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no')
            .setXFrameOptionsMode(HtmlService.XFrameOptionsMode.ALLOWALL);
}

/**********************************************************************************************************************************

**********************************************************************************************************************************/
function Include(filename) 
{
    return HtmlService.createHtmlOutputFromFile(filename).getContent();
}