# pip install pandas
# pip install matplotlib
# pip install skyfield
# pip install scipy
# pip install pytz

import pandas as pd
import matplotlib.pyplot as plt
import datetime
import numpy as np
from scipy.interpolate import griddata
from skyfield.api import load, Topos
import pytz
import math
import glob
from io import StringIO

########################################################################################################################
# This script has been tested on python 3.6
# Script configuration
folderpath = r""
filename = "*.csv"      # Input log file (see format below - log file columns description)
lat = 0.0               # terminal latitude [deg]
lon = 0.0               # terminal longitude [deg]
alt = 0.0               # terminal altitude [m]
min_sat_elevation = 5   # minimum satellite elevation to write in the csv file [deg]]
rssi_sat_detect_threshold = 4     # Minimum RSSI to display in the figures (4 = sat detection threshold)
rssi_sat_detect_max_display = 18  # Maximum RSSI to display in the figures
obs_window_start = datetime.datetime(2022, 2, 4, 00, 00, 00).replace(tzinfo=pytz.UTC)  # time start observation window
obs_window_end = datetime.datetime(2022, 2, 5, 23, 59, 59).replace(tzinfo=pytz.UTC)   # time end observation window
sat_lst = ['ASTROCAST-0101', 'ASTROCAST-0102', 'ASTROCAST-0103', 'ASTROCAST-0104', 'ASTROCAST-0105', 'ASTROCAST-0201', 'ASTROCAST-0202', 'ASTROCAST-0203', 'ASTROCAST-0204', 'ASTROCAST-0205']    # List of satellites used for the computation
########################################################################################################################
# Log file columns description
column = ["epoch", "sat_search_phase_cnt", "sat_detect_operation_cnt", "signal_demod_phase_cnt",
          "signal_demod_attempt_cnt", "signal_demod_success_cnt", "ack_demod_attempt_cnt",
          "ack_demod_success_cnt", "queued_msg_cnt", "dequeued_unack_msg_cnt", "ack_msg_cnt",
          "sent_fragment_cnt", "ack_fragment_cnt", "cmd_demod_attempt_cnt", "cmd_demod_success_cnt",
          "msg_in_queue", "ack_msg_in_queue", "last_rst",
          "last_mac_result", "last_sat_search_peak_rssi", "time_since_last_sat_search",
          "time_start_last_contact", "time_end_last_contact", "peak_rssi_last_contact",
          "time_peak_rssi_last_contact"]
########################################################################################################################


def get_gs_pass_info_from_tle(sat_tle, lat, lon, alt, elev_min, t_start, t_end):
    """
    Get satellite passes opportunities over a given terminal location for a given observation window
    :param sat_tle: Skyfield satellite object
    :param lat: latitude of terminal [deg]
    :param lon: longitude of terminal [deg]
    :param alt: altitude of terminal [m]
    :param elev_min: minimum satellite elevation to consider [deg]
    :param t_start: Observation window start time
    :param t_end: Observation window end time
    :return: Pandas dataframe with satellite passes opportunities
    """

    df = pd.DataFrame([])

    # Set ground-station / terminal location
    ground_station = Topos(latitude_degrees=float(lat),
                           longitude_degrees=float(lon),
                           elevation_m=alt)

    # Find contact oportunities
    ts = load.timescale()
    t, events = sat_tle.find_events(ground_station,
                                      ts.from_datetime(t_start - datetime.timedelta(minutes=10)),
                                      ts.from_datetime(t_end + datetime.timedelta(minutes=10)),
                                      altitude_degrees=0)

    difference = sat_tle - ground_station

    # Append to dataframe
    for ti, event in zip(t, events):

        alt, az, distance = difference.at(ti).altaz()

        if event == 0:
            aos_utc = ti.utc_datetime().replace(tzinfo=pytz.UTC)
            aos_elev = math.degrees(alt.radians)
        elif event == 1:
            max_utc = ti.utc_datetime().replace(tzinfo=pytz.UTC)
            max_elev = math.degrees(alt.radians)
        elif event == 2:
            los_utc = ti.utc_datetime().replace(tzinfo=pytz.UTC)
            los_elev = math.degrees(alt.radians)
            try:
                if max_elev >= elev_min:
                    df = df.append({'AOS UTC': aos_utc,
                                    'AOS el. [deg]': aos_elev,
                                    'AOS az. [deg]': None,
                                    'LOS UTC': los_utc,
                                    'LOS el. [deg]': los_elev,
                                    'LOS az. [deg]': None,
                                    'Duration [min]': (los_utc - aos_utc).total_seconds() / 60,
                                    'Max. el. UTC': max_utc,
                                    'Max. el. [deg]': max_elev,
                                    'Max. el. az. [deg]': None
                                    }, ignore_index=True)
            except:
                print('AOS not in time range, skipping. LOS was %s' % los_utc)

    return df


def get_satellite_elevation_azimuth_at_epoch_from_tle(sat_tle, lat, lon, alt, t_index):
    """
    Compute satellite location in polar coordinates for a given time index relatively to terminal location.
    :param sat_tle: Skyfield satellite object
    :param lat: latitude of terminal [deg]
    :param lon: longitude of terminal [deg]
    :param alt: altitude of terminal [m]
    :param t_index: Pandas time index for which satellite location has to be computed
    :return: Pandas dataframe with satellite location
    """

    df = pd.DataFrame(index=t_index, columns=['elevation', 'azimuth', 'distance'])

    # Set ground-station / terminal location
    ground_station = Topos(latitude_degrees=float(lat),
                           longitude_degrees=float(lon),
                           elevation_m=alt)

    difference = sat_tle - ground_station

    # For each epoch, get satellite location with respect to GS/terminal
    ts = load.timescale()
    for t in t_index:
        topocentric = difference.at(ts.from_datetime(t))
        s_alt, s_az, s_distance = topocentric.altaz()

        df.loc[t, 'elevation'] = s_alt.degrees
        df.loc[t, 'azimuth'] = s_az.degrees
        df.loc[t, 'distance'] = s_distance.m

    return df


def autolabel(rects):
    """
    Attach a text label above each bar in *rects*, displaying its height.
    Source: https://stackoverflow.com/questions/59873888/error-while-running-annonate-function-for-grouped-bar-chart-annotate-got-mult
    :param rects:
    :return: Empty
    """

    for rect in rects:
        height = rect.get_height()
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')


if __name__ == '__main__':

    df = pd.DataFrame([])

    # Parse log file from terminal (will load all log files matching pattern)
    dateparse = lambda x: datetime.datetime.utcfromtimestamp(int(x)).replace(tzinfo=pytz.UTC)
    for file in glob.glob(folderpath + filename):
        s = StringIO()
        with open(file) as f:
            for line in f:
                if line.startswith('HK'):
                    s.write(line[3:])
        s.seek(0)

        df = pd.concat([df, pd.read_csv(s, sep=';', names=column, parse_dates=['epoch'],
                                        date_parser=dateparse, index_col=['epoch'])])

    # Replace MAC result with human readable strings
    mac_result_dict = {0: "None",
                       1: "Success",
                       2: "Satellite not detected",
                       3: "Sync demodulation fail",
                       4: "Signaling demodulation fail",
                       5: "Ack signaling fail",
                       6: "No ack in frame",
                       7: "Error",
                       8: "Timeout",
                       9: "Blacklisted",
                       10: "Test satellite",
                       11: "Satellite low power interruption"}
    df = df.replace({"last_mac_result": mac_result_dict})

    # Trim observation window:
    df = df[(df.index > obs_window_start) & (df.index < obs_window_end)]

    # RSSI signal filtering (convolve with normalized Hanning window)
    windowSize = 8
    window = np.hanning(windowSize)
    window = window / window.sum()
    filtered = np.convolve(window, df['last_sat_search_peak_rssi'], mode='valid')
    df['last_sat_search_peak_rssi_filt'] = np.pad(filtered, (int(windowSize/2), int((windowSize/2)-1)), 'edge')

    # Generate satellite passes over location
    df["sat_elevation"] = 0
    df["sat_azimuth"] = 0
    df['sat_name'] = ''
    df['sat_in_view'] = False

    # Compute satellite passes time and elevation
    for sat in sat_lst:

        # Retrieve TLE file from Celestrak web site
        sat_tle_url = 'https://celestrak.com/NORAD/elements/gp.php?NAME=%s&FORMAT=TLE' % sat
        sat_tle = load.tle_file(sat_tle_url, reload=True, filename='%s.txt' % sat)[0]

        sat_pass_df = get_gs_pass_info_from_tle(sat_tle, lat, lon, alt, min_sat_elevation, df.index[0], df.index[-1])

        # filter timestamps -> valid only if in interval
        for epoch, sat_pass in sat_pass_df.iterrows():
            sat_in_view_tmp = (df.index > sat_pass['AOS UTC']) & (df.index < sat_pass['LOS UTC'])
            df['sat_in_view'] = df['sat_in_view'] | sat_in_view_tmp
            df.loc[sat_in_view_tmp, 'sat_name'] = sat

        # Compute satellite elevation for epochs during which the satellite is visible
        t_index_to_process = df[df['sat_name'] == sat].index
        df = df[~df.index.duplicated(keep='first')]
        sat_pass_df_cnt = get_satellite_elevation_azimuth_at_epoch_from_tle(sat_tle, lat, lon, alt, t_index_to_process)
        df.loc[t_index_to_process, 'sat_elevation'] = sat_pass_df_cnt['elevation']
        df.loc[t_index_to_process, 'sat_azimuth'] = sat_pass_df_cnt['azimuth']

    # Filtering data for which the satellite is in view and RSSI above detection threshold
    df_sat_in_view = df[(df['sat_elevation'] >= min_sat_elevation) &
                        (df['last_sat_search_peak_rssi'] >= rssi_sat_detect_threshold)]

    # Total fragments sent and statistics
    df_sat_in_view_reindex = df_sat_in_view['sent_fragment_cnt'].reindex(index=df.index)
    df_sat_in_view_reindex.interpolate(method='zero', inplace=True)
    df_ofst_sent = df['sent_fragment_cnt'] - df_sat_in_view_reindex
    df_ofst_sent = df_ofst_sent.fillna(0)
    df_ofst_sent = df_ofst_sent[df_ofst_sent > 0]
    df_ofst_sent_diff = df_ofst_sent.diff()
    df_ofst_sent_diff = df_ofst_sent_diff[df_ofst_sent_diff > 0]
    bin_sent_fragment_cnt_diff = df_sat_in_view['sent_fragment_cnt'].diff().fillna(0).groupby([(df_sat_in_view.index.date)]).sum()
    bin_sent_fragment_cnt_diff_corr = df_ofst_sent_diff.groupby([(df_ofst_sent_diff.index.date)]).sum()
    bin_sent_fragment_cnt_diff_corr = bin_sent_fragment_cnt_diff_corr.reindex(index=bin_sent_fragment_cnt_diff.index).fillna(0)
    bin_sent_fragment_cnt_diff = bin_sent_fragment_cnt_diff - bin_sent_fragment_cnt_diff_corr

    df_sat_in_view_reindex = df_sat_in_view['ack_fragment_cnt'].reindex(index=df.index)
    df_sat_in_view_reindex.interpolate(method='zero', inplace=True)
    df_ofst_ack = df['ack_fragment_cnt'] - df_sat_in_view_reindex
    df_ofst_ack = df_ofst_ack.fillna(0)
    df_ofst_ack = df_ofst_ack[df_ofst_ack > 0]
    df_ofst_ack_diff = df_ofst_ack.diff()
    df_ofst_ack_diff = df_ofst_ack_diff[df_ofst_ack_diff > 0]
    bin_ack_fragment_cnt_diff = df_sat_in_view['ack_fragment_cnt'].diff().fillna(0).groupby([(df_sat_in_view.index.date)]).sum()
    bin_ack_fragment_cnt_diff_corr = df_ofst_ack_diff.groupby([(df_ofst_ack_diff.index.date)]).sum()
    bin_ack_fragment_cnt_diff_corr = bin_ack_fragment_cnt_diff_corr.reindex(index=bin_ack_fragment_cnt_diff.index).fillna(0)
    bin_ack_fragment_cnt_diff = bin_ack_fragment_cnt_diff - bin_ack_fragment_cnt_diff_corr

    # Data plotting - RSSI polar heat map (with interpolated)
    rad_grid = np.arange(min_sat_elevation, 95, 5)
    azm_grid = np.arange(0, 365, 5)
    RAD, AZM = np.meshgrid(rad_grid, azm_grid)

    rad = df_sat_in_view['sat_elevation']
    azm = df_sat_in_view['sat_azimuth']
    z = df_sat_in_view['last_sat_search_peak_rssi']
    coord_xy = np.array([rad, azm])
    grid = griddata(coord_xy.transpose(), z.values, (RAD, AZM), method='nearest')

    actual = np.radians(azm_grid)
    expected = np.arange(min_sat_elevation, 95, 5)
    r, theta = np.meshgrid(expected, actual)
    values = grid
    fig, ax = plt.subplots(subplot_kw=dict(projection='polar'))
    ax.set_aspect(1.0)
    ax.set_rlim(90, min_sat_elevation, 1)
    ax.set_theta_zero_location("N")
    ax.set_theta_direction(-1)
    ax.set_rlim(bottom=90, top=min_sat_elevation)
    contourf_ = ax.contourf(theta, r, grid, cmap='coolwarm', levels=np.linspace(rssi_sat_detect_threshold,
                                                                                rssi_sat_detect_max_display))
    fig.colorbar(contourf_)
    plt.title("RSSI")
    plt.savefig(folderpath + "rssi_heat_map_polar.png")
    plt.savefig(folderpath + "rssi_heat_map_polar.svg")
    plt.close()

    # Data plotting - RSSI polar heat map discrete
    fig = plt.figure()
    ax = fig.add_subplot(polar=True)
    ax.set_aspect(1.0)
    ax.set_rlim(90, min_sat_elevation, 1)
    ax.set_theta_zero_location("N")
    ax.set_theta_direction(-1)
    plt.scatter(azm * np.pi / 180, rad, c=z, cmap='coolwarm')
    plt.clim(rssi_sat_detect_threshold, rssi_sat_detect_max_display)
    plt.colorbar()
    plt.title("RSSI")
    plt.savefig(folderpath + "rssi_heat_map_disctrete.png")
    plt.savefig(folderpath + "rssi_heat_map_disctrete.svg")
    plt.close()

    # Data plotting - MAC = SUCCESS polar discrete
    fig = plt.figure()
    ax = fig.add_subplot(polar=True)
    ax.set_aspect(1.0)
    ax.set_rlim(90, min_sat_elevation, 1)
    ax.set_theta_zero_location("N")
    ax.set_theta_direction(-1)
    plt.scatter(df_sat_in_view['sat_azimuth'][df_sat_in_view['last_mac_result'] == 'Success'] * np.pi / 180,
                df_sat_in_view['sat_elevation'][df_sat_in_view['last_mac_result'] == 'Success'])
    plt.title("MAC result = SUCCESS")
    plt.savefig(folderpath + "mac_success_polar.png")
    plt.savefig(folderpath + "mac_success_polar.svg")
    plt.close()

    #  Data plotting - time plot
    f, (a0, a1, a2) = plt.subplots(3, 1, gridspec_kw={'height_ratios': [3, 1, 1]}, sharex=True, figsize=(20,13))
    a0.plot(df['last_sat_search_peak_rssi_filt'], color='black', alpha=0.7)
    for mac_result in mac_result_dict:
        a0.plot(df['last_sat_search_peak_rssi'][df['last_mac_result'] == mac_result_dict[mac_result]],
                marker='.', linestyle='none', label=mac_result_dict[mac_result])
    a0.set_ylabel('RSSI [dB]')
    a0.set_title('Last satellite search peak RSSI and MAC result')
    a0.grid(True)
    a0.legend()
    for sat in sat_lst:
        a1.plot(df['sat_elevation'][df['sat_name'] == sat], marker='.', linestyle='none', label=sat)
    a1.set_ylabel('Elevation [deg]')
    a1.set_title('Satellite elevation')
    a1.grid(True)
    a1.legend()
    a2.plot(df.ack_fragment_cnt)
    a2.set_xlabel('Gregorian date')
    a2.set_ylabel('counter [-]')
    a2.set_title('ACK fragment counter')
    a2.grid(True)
    plt.savefig(folderpath + "rssi_mac_elev_counter.png")
    plt.savefig(folderpath + "rssi_mac_elev_counter.svg")
    plt.close()

    # Data plotting - Histogram sent/ack fragments
    fig, ax = plt.subplots(figsize=(10, 8))
    width = 0.2  # width of bar
    p1 = ax.bar(bin_sent_fragment_cnt_diff.index, bin_sent_fragment_cnt_diff.values, width, label='sent_fragment_cnt')
    p2 = ax.bar(bin_ack_fragment_cnt_diff.index, bin_ack_fragment_cnt_diff.values, width, label='ack_fragment_cnt')
    ax.legend()
    ax.set_xticks(bin_sent_fragment_cnt_diff.index)
    plt.grid(True, 'major', 'y', ls='--', lw=.5, c='k', alpha=.3)
    plt.title("Sent fragments %d - ACK fragments %d - Ratio: %.1f\n"
              "Observation window: %s - %s (%.1fH)" % (bin_sent_fragment_cnt_diff.sum(),
                                                       bin_ack_fragment_cnt_diff.sum(),
                                                       bin_sent_fragment_cnt_diff.sum()/bin_ack_fragment_cnt_diff.sum(),
                                                       df.index[0],
                                                       df.index[-1],
                                                       (df.index[-1]-df.index[0]).total_seconds()/3600))
    autolabel(p1)
    autolabel(p2)
    plt.xlabel("Gregorian date")
    plt.ylabel("count [-]")
    plt.xticks(rotation=10)
    plt.savefig(folderpath + "histogram_fragments_sent_in_view_container.png")
    plt.savefig(folderpath + "histogram_fragments_sent_in_view_container.svg")
    plt.close()
