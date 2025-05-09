import matplotlib.pyplot as plt
import pandas as pd
from scipy.stats import linregress

df = pd.read_csv("data/Testing Data Sheet - Data for Plotting.csv")

reactivity_cols = ["Handheld (%) - REAC", "Sens1 - REAC", "Sens2 - REAC", "Sens3 - REAC", "Sens4 - REAC"]

print("Linear Regression Coefficients (Slope, Intercept, R-value):")
plt.figure()
for sens in reactivity_cols:
    plt.plot(df["Handheld (%) - REAC"], df[sens], marker = "o", label=sens.split()[0])
    slope, intercept, r_value, p_value, std_err = linregress(df["Handheld (%) - REAC"].dropna(), df[sens].dropna())
    print(f"{sens.split()[0]}: slope={slope:.4f}, intercept={intercept:.4f}, r_value={r_value:.4f}")
plt.fill_between(df["Handheld (%) - REAC"], (df["Handheld (%) - REAC"]-10).clip(lower=0), (df["Handheld (%) - REAC"]+10).clip(upper=100), color="gray", alpha=0.2, label="±10% Accuracy Target")


plt.xlabel("Handheld (%)")
plt.ylabel("Sensor Moisture Reading (%)")
plt.title("Moisture Sensor Readings vs Handheld Reference")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()


consistency_cols = ["Hanheld (%) - CONS", "Sens1 - CONS", "Sens2 - CONS", "Sens3 - CONS", "Sens4 - CONS"]

# Plot boxplots
df.boxplot(column=consistency_cols, grid=False)
plt.title("Box Plots of Handheld and Moisture Sensors")
plt.xticks([1, 2, 3, 4, 5], ["Handheld", "Sensor 1", "Sensor 2", "Sensor 3", "Sensor 4"])
plt.xlabel("Sensor")
plt.ylabel("Percentage")
plt.tight_layout()
plt.show()

# Calculate and print standard deviations
std_devs = df[consistency_cols].std()
print("Standard Deviations:")
print(std_devs)
