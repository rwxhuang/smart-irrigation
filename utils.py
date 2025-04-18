import requests
import streamlit as st


def get_coordinates(city_name):
    url = f"https://nominatim.openstreetmap.org/search?q={city_name}&format=json&limit=1"
    headers = {"User-Agent": "WeatherDashboardApp/1.0 (contact@example.com)"}
    response = requests.get(url, headers=headers)

    if response.status_code == 200:
        location_data = response.json()
        if location_data:
            location = location_data[0]
            return float(location['lat']), float(location['lon'])
        else:
            st.warning(
                "City not found. Try adding the country name (e.g., 'Paris, France').")
            return None, None
    else:
        st.error(
            f"API request failed with status code {response.status_code}: {response.text}")
        return None, None


def get_weather_data(lat, lon, hours):
    url = f"https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={lon}&current=temperature_2m,relative_humidity_2m,is_day,precipitation,rain&daily=shortwave_radiation_sum"
    response = requests.get(url)

    if response.status_code == 200:
        return response.json()
    else:
        st.error("Failed to retrieve weather data.")
        return None
    

def get_particle_data(api_endpoint = "allDataAPI", **kwargs):
    device_id = "420027000d47373336373936"

    url = f"https://api.particle.io/v1/devices/{device_id}/{api_endpoint}"

    response = requests.get(url, **kwargs)

    if response.status_code == 200:
        data = response.json()
        print(f"{api_endpoint} value: {data['result']}")
        
        return data
    else:
        print("Error:", response.status_code, response.text)
        raise Exception("Failed to retrieve particle data.")
