from fastapi import FastAPI
import paho.mqtt.client as mqtt
import json
import asyncio
import asyncpg

app = FastAPI()

#---------------configuración-----------------#
BROKER = '192.168.1.16'
PORT = 1883
TOPIC = 'sensor/datos'

DB_URL = "postgresql://neondb_owner:npg_bLi73eBCxmqP@ep-dawn-firefly-anj8wtz6-pooler.c-6.us-east-1.aws.neon.tech/neondb?sslmode=require&channel_binding=require"

db = None
loop = None
#---------------MQTT -----------------#
def on_connect(client, Userdata, flags, rc):
    print("Conectado a MQTT")
    client.subscribe(TOPIC)

def on_message(client, userdata, msg):
    payLoad = msg.payload.decode()
    print (f"Mensaje recibido: {payLoad}")

    try:
        data = json.loads(payLoad)

        asyncio.run_coroutine_threadsafe(save_to_db(data), loop)

    except Exception as e:
        print(f"Error al procesar el mensaje: {e}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message


#---------------Base de datos-----------------#
async def save_to_db(data):
    try:
        await db.execute(
            """
            INSERT INTO sensores(temperatura, humedad, luz)
            VALUES($1, $2, $3)
            """,
            data["temperatura"],
            data["humedad"],
            data["luz"]
        )
        print("Datos guardados en la base de datos")

    except Exception as e:
        print(f"Error al guardar en la base de datos: {e}")

#---------------FasTAPI-----------------------#

@app.on_event("startup")
async def startup():
    global db, loop
    loop = asyncio.get_event_loop()
    db = await asyncpg.connect(DB_URL)
    print("Conectado a la base de datos")

    client.connect(BROKER, PORT, 60)
    client.loop_start()

@app.on_event("shutdown")
async def shutdown():
    client.loop_stop()
    await db.close()

    #---------#
@app.get("/data")
async def get_data():
    rows = await db.fetch("SELECT * FROM sensores ORDER BY id DESC LIMIT 10")
    return [[dict(r)for r in rows]]

#------------------#
@app.get("/")
def root():
    return {"message": "API funcionando"}