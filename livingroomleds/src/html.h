#include <ESP8266WiFi.h>

const char htmlTemplate[] PROGMEM = \
R"(
<form method="post">
    Red:<input type="number" name="red" size="3" value=%d>
    Green:<input type="number" name="green" size="3" value=%d>
    Blue:<input type="number" name="blue" size="3" value=%d>
    Brightness:<input type="number" name="brightness" size="3" value=%d>
    <input type="submit" value="Send">
    <button type="submit" name="rainbow" value="true">Rainbow</button>
</form>

<style>
.input,body {
    text-align:center
    border:#344e9D;
    height:40px;
    width:30px;
    border-radius:20px
}
body {
    background:#344e5c;
    font-family:sans-serif;
    color:#FFF
}
</style>
)";