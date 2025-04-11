import pandas as pd
import numpy as np
from datetime import datetime, timedelta

# Settings
num_points = 2000
num_days = 5
plants = ['Chives', 'Spinach']
start_time = datetime(2025, 4, 1)

# Generate timestamps evenly spread over 5 days
time_interval = (num_days * 24 * 60) / (num_points / len(plants))  # in minutes
timestamps = [start_time + timedelta(minutes=i * time_interval)
              for i in range(num_points // len(plants))]

# Generate soil moisture data with slight variation per plant
np.random.seed(42)
chives_moisture = np.clip(np.random.normal(
    loc=22.0, scale=1.5, size=num_points // 2), 15, 30)
spinach_moisture = np.clip(np.random.normal(
    loc=25.0, scale=1.2, size=num_points // 2), 18, 32)

# Create dataframes
df_chives = pd.DataFrame({
    'TIMESTAMP': timestamps,
    'PLANT': 'Chives',
    'SOIL_MOISTURE': chives_moisture
})

df_spinach = pd.DataFrame({
    'TIMESTAMP': timestamps,
    'PLANT': 'Spinach',
    'SOIL_MOISTURE': spinach_moisture
})

# Combine and shuffle
df = pd.concat([df_chives, df_spinach]).sample(
    frac=1, random_state=42).reset_index(drop=True)

# Save to CSV with UTF-8 encoding
file_path = "./data/soil_moisture_data.csv"
df.to_csv(file_path, index=False, encoding='utf-8')

file_path
