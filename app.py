import time  # to simulate a real time data, time loop
import numpy as np  # np mean, np random
import pandas as pd  # read csv, df manipulation
# import plotly.express as px  # interactive charts
import streamlit as st  # 🎈 data web app development
from utils import *
import requests
from datetime import datetime
import os
from streamlit_autorefresh import st_autorefresh


# read csv from a URL

st.set_page_config(
    page_title="Real-Time Micro-irrigation System Dashboard",
    page_icon="🍃",
)

# Create logs folder if not exists
os.makedirs("logs", exist_ok=True)
log_filename = "logs/live_data_log.csv"

# Thresholds (ALERTS)
FLOW1_MIN = 0.10  # L/m
FLOW2_MIN = 0.10
FLOW1_MAX = 5.00  # L/m
FLOW2_MAX = 5.00
SOIL_MIN = 40.0   # %
SOIL_MAX = 80.0

@st.cache_data(ttl=300)
def get_weather_data_cached(lat, lon, hours=1):
    return get_weather_data(lat, lon, hours)


@st.cache_data(ttl=60)
def get_live_data() -> pd.DataFrame:
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
        value_headers = [
            "Soil1", "Soil2", "Soil3", "Soil4",
            "ExtTemp_C", "ExtRH", "ExtTemp_F",
            "IntTemp_C", "IntRH", "IntTemp_F",
            "Flow1", "Flow2"
        ]
        raw_values = data["result"].split(",")

        # Convert to float safely, replacing '' with np.nan
        values = [float(v) if v.strip() != '' else np.nan for v in raw_values]
        df = pd.DataFrame([values], columns=value_headers)
        df["TIMESTAMP"] = datetime.now()
        return df
    else:
        st.error(f"API Error {response.status_code}: {response.text}")
        return pd.DataFrame()


live_data = pd.DataFrame()

# --- Dashboard Header ---
st.title("Real-Time Soil Moisture Dashboard")

city_name = st.text_input("City Name", value="Kandahar")
lat, lon = get_coordinates(city_name)
if lat and lon:
    data = get_weather_data(lat, lon, hours=24)

st.subheader("Current Weather Summary")
col1, col2, col3, col4 = st.columns(4)
col1.metric("🌡️ Temperature", f"{data['current']['temperature_2m']}°C")
col2.metric("💧 Humidity", f"{data['current']['relative_humidity_2m']}%")
col3.metric("🌧️ Precipitation", f"{data['current']['precipitation']} mm")
col4.metric("☀️ Solar (kWh/m^2)",
            f"{round(data['daily']['shortwave_radiation_sum'][0] / 3.6, 1)}")

# --- Real-time Visualization ---
if "live_data" not in st.session_state:
    st.session_state.live_data = pd.DataFrame()
placeholder = st.empty()

# 🔄 Automatically refresh every 15 seconds
st_autorefresh(interval=15_000, limit=None, key="live-refresh")

sensor_row = get_live_data()

if lat and lon:
    weather_data = get_weather_data_cached(lat, lon, hours=1)
else:
    weather_data = {}

if sensor_row.empty:
    time.sleep(15)
else:
    now = datetime.now()

    # Extract forecast values at this timestamp (using nearest time)
    if "hourly" in weather_data:
        hourly_df = pd.DataFrame(weather_data["hourly"])
        hourly_df["time"] = pd.to_datetime(hourly_df["time"])
        nearest_idx = hourly_df["time"].sub(now).abs().idxmin()
        temp = hourly_df.loc[nearest_idx, "temperature_2m"]
        rh = hourly_df.loc[nearest_idx, "relative_humidity_2m"]
    else:
        temp = np.nan
        rh = np.nan

    # Append to sensor row
    sensor_row["Weather_Temp_C"] = temp
    sensor_row["Weather_RH"] = rh

    # Log and store
    st.session_state.live_data = pd.concat([st.session_state.live_data, sensor_row], ignore_index=True)
    live_data = st.session_state.live_data
    sensor_row.to_csv(log_filename, mode='a', header=not os.path.exists(log_filename), index=False)

# Display charts
with placeholder.container():
    latest = sensor_row.iloc[0]
    st.markdown("### 🌡️ Temperature and Humidity")

    sensor_cols = ["ExtTemp_C", "ExtRH", "IntTemp_C", "IntRH", "Weather_Temp_C", "Weather_RH"]
    sensor_labels = {
        "ExtTemp_C": "External Temp (°C)",
        "ExtRH": "External RH (%)",
        "IntTemp_C": "Internal Temp (°C)",
        "IntRH": "Internal RH (%)",
        "Weather_Temp_C": "Forecast Temp (°C)",
        "Weather_RH": "Forecast RH (%)"
    }
    st.line_chart(live_data.set_index("TIMESTAMP")[sensor_cols].rename(columns=sensor_labels))

    col_ext1, col_ext2, col_ext3 = st.columns(3)
    col_ext1.metric("🌤️ External Temp (°C)", f"{latest['ExtTemp_C']:.1f}")
    col_ext2.metric("💧 External RH (%)", f"{latest['ExtRH']:.1f}")
    col_ext3.metric("🌤️ External Temp (°F)", f"{latest['ExtTemp_F']:.1f}")

    col_int1, col_int2, col_int3 = st.columns(3)
    col_int1.metric("🏠 Internal Temp (°C)", f"{latest['IntTemp_C']:.1f}")
    col_int2.metric("💧 Internal RH (%)", f"{latest['IntRH']:.1f}")
    col_int3.metric("🏠 Internal Temp (°F)", f"{latest['IntTemp_F']:.1f}")


    st.markdown("### 💧 Soil Moisture Monitoring")
    st.line_chart(live_data.set_index("TIMESTAMP")[["Soil1", "Soil2", "Soil3", "Soil4"]])

    colm1, colm2, colm3, colm4 = st.columns(4)
    colm1.metric("Soil 1 (%)", f"{latest['Soil1']:.1f}")
    colm2.metric("Soil 2 (%)", f"{latest['Soil2']:.1f}")
    colm3.metric("Soil 3 (%)", f"{latest['Soil3']:.1f}")
    colm4.metric("Soil 4 (%)", f"{latest['Soil4']:.1f}")

    st.markdown("### 💧 Flow Rate Monitoring")
    st.line_chart(live_data.set_index("TIMESTAMP")[["Flow1", "Flow2"]])

    colf1, colf2 = st.columns(2)
    colf1.metric("Flow 1 (L/m)", f"{latest['Flow1']:.1f}")
    colf2.metric("Flow 2 (L/m)", f"{latest['Flow2']:.1f}")

    # --- Alerts ---
    alerts = []
    for i in range(1, 5):
        if latest[f"Soil{i}"] < SOIL_MIN:
            alerts.append(f"⚠️ Soil Moisture {i} too low: {latest[f'Soil{i}']:.1f}%")
        elif latest[f"Soil{i}"] > SOIL_MAX:
            alerts.append(f"⚠️ Soil Moisture {i} too high: {latest[f'Soil{i}']:.1f}%")

    if latest["Flow1"] > FLOW1_MAX:
        alerts.append(f"🚨 Flow1 too high: {latest['Flow1']:.1f} L/m")
    elif latest["Flow1"] < FLOW1_MIN:
        alerts.append(f"🚨 Flow1 too low: {latest['Flow1']:.1f} L/m")
    if latest["Flow2"] > FLOW2_MAX:
        alerts.append(f"🚨 Flow2 too high: {latest['Flow2']:.1f} L/m")
    elif latest["Flow2"] < FLOW2_MIN:
        alerts.append(f"🚨 Flow2 too low: {latest['Flow2']:.1f} L/m")

    if alerts:
        st.warning("### ⚠️ Alerts")
        for alert in alerts:
            st.write(alert)

    st.markdown("### 📥 Export Logged Data")
    csv = live_data.to_csv(index=False).encode('utf-8')
    st.download_button("Download CSV", data=csv, file_name="live_data_log.csv", mime='text/csv')

