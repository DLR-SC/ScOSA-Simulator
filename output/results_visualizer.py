# -*- coding: utf-8 -*-
import pandas as pd
import visualizer_tools as vis

vis.clearConsole()

# -------INPUTS--------
scen_name = "blocking_example"
scen_num = "0"
x_bounds_tasks = (0,2)
x_bounds_usage = (0.9,0.90030)
save_plots = True
# ---------------------


dir_name = scen_name + "_" + scen_num
print("Opening directory " + dir_name + "...")

### Load Data    
# -Delays
delays = pd.read_csv(dir_name + '/delays.csv')

# -Task Triggering
task_trigger = pd.read_csv(dir_name + '/task_trigger.csv')

# -Port Assignment
port_assignment = pd.read_csv(dir_name + '/port_assignment.csv')
    
# -Message Transmission 
transmission = pd.read_csv(dir_name + '/transmission.csv')
    
### Plot Data
# -Message Delay Histogram
#vis.plot_delay_hist(delays, False, save_plots)

# -Message Delay Histogram (Normalized)
#vis.plot_delay_hist(delays, True, save_plots)

# -Task Triggering
vis.plot_task_trigger(task_trigger, x_bounds_tasks, save_plots)

# -Channel Usage
vis.plot_link_usage(port_assignment, transmission, x_bounds_usage, save_plots)

# -Traffic Metrics 
traffic = vis.calc_link_usage(port_assignment, transmission)