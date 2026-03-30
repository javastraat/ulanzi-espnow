How to use with curl

Form style:
curl -X POST http://ULANZI_IP/api/display/message -d "text=Hello from curl"

With icon and beep:
curl -X POST http://ULANZI_IP/api/display/message -d "text=Doorbell" -d "icon=/icons/bell.gif" -d "beep=true"

JSON style:
curl -X POST http://ULANZI_IP/api/display/message -H "Content-Type: application/json" -d "{"text":"Dinner ready","icon":"/icons/bell.gif","beep":true}"
