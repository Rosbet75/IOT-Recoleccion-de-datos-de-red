import requests
import matplotlib

var = requests.get("http://10.203.84.232/extractor.php?limit=1000").json()

print(var)
