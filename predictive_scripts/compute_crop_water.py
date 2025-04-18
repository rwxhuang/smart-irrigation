
def compute_crop_water(temperature, radiation, relative_humidity, crop_coeff):
    """
    All measurements should be averaged over a time span of interest

    temperature: degrees celcius
    radiation: kwh / m^2 / day
    relative_humidity: percent
    precipitation: mm / day
    crop_coeff: 

    Computes the amount of water required for crop growth over a day per meter^2
    units: mm / day
    """
    # constants:
    ELEVATION = 3310 # elevation above sea level (feet), 1,010 meters
    DAYTIME_WIND_SPEED = 2 # wind speed during day (mph)

    # convert units
    temperature = temperature * 9/5 + 32 # convert to fahrenheit
    radiation = 85.98 * radiation

    # lambda = heat of vaporization of water
    lambda_ = 1543 - 0.796 * temperature

    # delta = slope of vapor pressure curve
    delta = 0.051 * ((164.8 + temperature) / 157)**7

    # bp = mean barometric pressure
    bp = 1.013 * (1 - ELEVATION / 145350)**5.26

    # gamma = psychrometric constant
    gamma = 0.339 * bp / (0.622 * lambda_)

    # b_r = adjustment factor
    b_r = 1.06 - 0.0013 * relative_humidity \
        + 8.38 * 10**-4 * DAYTIME_WIND_SPEED \
        - 3.73 * 10**-6 * relative_humidity * DAYTIME_WIND_SPEED \
        - 0.315 * 10**-4 * relative_humidity**2 \
        - 3.82 * 10**-7 * DAYTIME_WIND_SPEED**2

    evapotranspiration = -0.012 + (delta / (delta + gamma))*b_r * radiation / lambda_

    crop_water = crop_coeff * evapotranspiration # crop water in in/day
    
    # convert units
    crop_water = crop_water * 25.4 # mm/day

    return crop_water

