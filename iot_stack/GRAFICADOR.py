import requests
import matplotlib.pyplot as plt
import datetime
from datetime import datetime
try:
    var = requests.get("http://10.203.84.232/extractor.php?limit=1000").json()
except Exception as e:
    print("Error fetching data:", e)
    exit()

if not var:
    print("mamo verga mi hermano")
    exit()
timestamps = [datetime.strptime(x['fecha'], "%Y-%m-%d %H:%M:%S") for x in var]

columnas = ['rssi', 'temp', 'hum', 'distancia', 'touch4', 'touch5']
data = {}
for col in columnas:
    try:
        if col.startswith("touch") or col == "rssi":
            data[col] = [int(x[col]) for x in var]
        else:
            data[col] = [float(x[col]) for x in var]
    except KeyError:
        print("mamo verga mi hermano: {col} ",{e})
        data[col] = [0] * len(var)
    
    
fig, axs = plt.subplots(len(columnas), 1, figsize=(12, 3*len(columnas)), sharex=True)

for i, col in enumerate(columnas):
    axs[i].plot(timestamps, data[col], marker='o', linestyle='-', label=col)
    axs[i].set_ylabel(col)
    axs[i].legend()
    axs[i].grid(True)

# Configurar eje X
axs[-1].set_xlabel("Fecha y hora")
fig.autofmt_xdate()  # rotar fechas
plt.tight_layout()
plt.show()