import time  # to simulate a real time data, time loop
import numpy as np  # np mean, np random
import pandas as pd  # read csv, df manipulation
# import plotly.express as px  # interactive charts
import streamlit as st  # 🎈 data web app development
from utils import *

# read csv from a URL

st.set_page_config(
    page_title="Real-Time Micro-irrigation System Dashboard",
    page_icon="🍃",
)


@st.cache_data
def get_data() -> pd.DataFrame:
    df = pd.read_csv('./data/soil_moisture_data.csv', encoding='utf-8')
    df['TIMESTAMP'] = pd.to_datetime(df['TIMESTAMP'])
    df = df.sort_values('TIMESTAMP')
    return df

    """
    Example using API:
    headers = {
        "Authorization": f"Bearer {st.secrets['access_token']}"
    }
    data = utils.get_particle_data(api_endpoint = "allDataAPI", headers=headers)
    df = pd.DataFrame(data)
    return df
    """


df = get_data()
# dashboard title
st.title("Real-Time / Live Soil Moisture Dashboard")

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

st.markdown('### Flow Rates')
cols = st.columns(3)
cols[0].metric("PRIMARY_TUBE (mL/s)", 31.7, delta=0)
cols[1].metric("CHIVE_TUBE (mL/s)", 25.8, delta=-0.6)
cols[2].metric("SPINACH_TUBE (mL/s)", 25.2, delta=0.8)

st.markdown("### Soil Moisture Levels")
plants_filter = st.selectbox("Select the Plant", pd.unique(df["PLANT"]))
df = df[df["PLANT"] == plants_filter]

placeholder = st.empty()
for seconds in range(200):
    with placeholder.container():
        st.line_chart(df.set_index("TIMESTAMP")[
            "SOIL_MOISTURE"][seconds:seconds+10])
        time.sleep(1)
