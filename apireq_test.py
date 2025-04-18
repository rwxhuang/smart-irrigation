import requests
import streamlit as st

device_id = "420027000d47373336373936"
function_name = "set_interval"
interval = "15000"
variable_name = "allDataAPI"

url = f"https://api.particle.io/v1/devices/{device_id}/{variable_name}"
headers = {
    "Authorization": f"Bearer {st.secrets['access_token']}"
}

response = requests.get(url, headers=headers)

if response.status_code == 200:
    data = response.json()
    print(f"{variable_name} value: {data['result']}")
    print()
    value_headers = ["Soil Moisture (1)", "Soil Moisture (1)", "Soil Moisture (1)", "Soil Moisture (1)", "Temperature (C)", "RH", "Flow Rate (1)"]
    for i,j in zip(value_headers, data['result'].split(",")):
        print(f"{i}: {j}")
else:
    print("Error:", response.status_code, response.text)



# url = f"https://api.particle.io/v1/devices/{device_id}/{function_name}"
# headers = {
#     "Authorization": f"Bearer {access_token}"
# }
# data = {
#     "args": interval  # You must include this, even if your function doesn't use it
# }

# response = requests.post(url, headers=headers, data=data)

# if response.status_code == 200:
#     result = response.json()
#     print("Function call result:", result)
# else:
#     print("Error:", response.status_code, response.text)