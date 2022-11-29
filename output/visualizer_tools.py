# -*- coding: utf-8 -*-
import numpy as np
import matplotlib.pyplot as plt
import os

colors = ['tab:blue',
          'tab:orange',
          'tab:green',
          'tab:red',
          'tab:purple',
          'tab:brown',
          'tab:pink',
          'tab:gray',
          'tab:olive',
          'tab:cyan',]

def clearConsole():
    command = 'clear'
    if os.name in ('nt', 'dos'):  # If Machine is running on Windows, use cls
        command = 'cls'
    os.system(command)
    
def test_f():
    x = [1,2,3,4,5]
    return (x,2)

def get_tasks(data_csv):
    data = data_csv.to_numpy()
    names = data[:,3]
    
    unique_names = [];
    for name in names:
        if name in unique_names: continue
        else: unique_names.append(name)
    return (len(unique_names), unique_names)

def get_nodes(data_csv):
    data = data_csv.to_numpy()
    names = data[:,1]
    
    unique_names = [];
    for name in names:
        if name in unique_names: continue
        else: unique_names.append(name)
    return (len(unique_names), unique_names)
    
def plot_delay_hist(delays, norm=False, save=False):
    # -Message Delay Histogram
    if norm == True:    
        counts, bins = np.histogram(delays.to_numpy()[:,9]/delays.to_numpy()[:,6])
    else:
        counts, bins = np.histogram(delays.to_numpy()[:,9])
        
    fig, ax = plt.subplots()
    delay_fig = plt.hist(bins[:-1], bins, weights=counts)
    ax.set_ylabel('Occurrances [-]')
    ax.grid(True)
    if norm == True:    
        ax.set_xlabel('Message Delay/Message Size [s/byte]')
        ax.set_title('Message Delay Distribution - Normalized')
    else:
        ax.set_xlabel('Message Delay [s]')
        ax.set_title('Message Delay Distribution')
    
    if save==True: plt.savefig("delays.png")
    
def get_task_trigger(data_csv, task_name, node_name):
    data = data_csv.to_numpy()
    
    out = [];
    for i in range(len(data)):
        if data[i,3] == task_name and data[i,1] == node_name:
            start = data[i,4]
            end = data[i,5]
            dur = end-start
            out.append((start,dur))
    return out    
    
def plot_task_trigger(task_trigger, x_lim, save):
    fig, ax = plt.subplots()
    n_tasks, task_names = get_tasks(task_trigger)
    n_nodes, node_names = get_nodes(task_trigger)
    tics = [];
    
    for i in range(n_tasks):
        task_name = task_names[i]
        
        for j in range(n_nodes):
            node_name = node_names[j]
            triggers = get_task_trigger(task_trigger, task_name, node_name)
            
            y = (i*1.25, 1)
            ax.broken_barh(triggers, y, facecolors=colors[j])
        tics.append(i*1.25+1.25/2)
        
    ax.set_yticks(tics)
    ax.set_yticklabels(task_names)
    ax.grid(True)
    ax.set_xlabel('Simulation time [s]')
    ax.set_title('Task Activation Pattern')
    ax.legend(node_names)
    ax.set_xlim(x_lim)
    
    if save==True: plt.savefig("task_trigger.png")

def get_links(data_csv):
    data = data_csv.to_numpy()
    unique_links = [];
    
    for i in range(len(data)):
        if data[i,0] != -1 and data[i,2] != -1:
            sender = data[i,1]
            dest = data[i,3]
            
            if unique_links.count( (sender, dest) ) > 0 or unique_links.count( (dest, sender) ):
                continue
            unique_links.append((sender,dest))
    
    return (len(unique_links), unique_links)
        
def get_link_assignment(port_assignment, pair):
    data = port_assignment.to_numpy()
    sender, dest = pair
    
    out = []
    for i in range(len(data)):
        if (data[i,1] == sender and data[i,3] == dest) or (data[i,1] == dest and data[i,3] == sender):
            start = data[i,4]
            dur = data[i,6]
            out.append((start,dur))
    return out

def get_link_use(transmission, pair):
    data = transmission.to_numpy()
    sender, dest = pair
    
    out = []
    for i in range(len(data)):
        if (data[i,1] == sender and data[i,3] == dest) or (data[i,1] == dest and data[i,3] == sender):
            start = data[i,4]
            dur = data[i,6]
            out.append((start,dur))
    return out

def plot_link_usage(port_assignment, transmission, x_lim, save):
    fig, ax = plt.subplots()
        
    n_links, links = get_links(port_assignment)
    tics = []
    for i in range(n_links):
        
        pair = links[i]
        assignment = get_link_assignment(port_assignment, pair)  
        useage = get_link_use(transmission, pair)
        
        y_assignment = (i*1.25, 1)
        y_usage = (i*1.25+0.25/2,0.75)
        ax.broken_barh(assignment, y_assignment, facecolors=colors[0])
        ax.broken_barh(useage, y_usage, facecolors=colors[1])
        tics.append(i*1.25+1.25/2)
    
    ax.set_yticks(tics)
    ax.set_yticklabels(links)
    ax.grid(True)
    ax.set_xlabel('Simulation time [s]')
    ax.set_title('Link Usage Pattern')
    ax.legend(["Link Established", "Data Transmitted"])
    ax.set_xlim(x_lim)
    
    if save==True: plt.savefig("link_usage.png")

def calc_link_usage(port_assignment, transmission):
    assignment_data = port_assignment.to_numpy();
    transmission_data = transmission.to_numpy();
    
    traffic = {};
    for i in range(len(assignment_data)):
        tx_data = 0;
        link_start = assignment_data[i,4]
        link_end = assignment_data[i,5]
        link_dur = assignment_data[i,6]
        link_sender = assignment_data[i,0]
        link_dest = assignment_data[i,2]
        task_name = ""
        
        if link_sender == -1 or link_dest == -1: continue
    
        if (link_sender, link_dest) not in traffic and (link_dest, link_sender) not in traffic:
            traffic[(link_sender, link_dest)] = {}
        
        for j in range(len(transmission_data)):
            task = transmission_data[j,8]
            tx_sender= transmission_data[j,0]
            tx_dest= transmission_data[j,2]
            tx_start = transmission_data[j,4]
            tx_end = transmission_data[j,5]
            
            if tx_sender == -1 or tx_dest == -1: continue
            
            if (link_start <= tx_start) and (tx_end <= link_end) and (link_sender == tx_sender) and (link_dest == tx_dest):
                task_name = task
                if (link_sender, link_dest) not in traffic:
                    if task_name not in traffic[(link_dest,link_sender)]:
                        traffic[(link_dest,link_sender)][task_name] = []
                else:
                    if task_name not in traffic[(link_sender,link_dest)]:
                        traffic[(link_sender,link_dest)][task_name] = []
                
                tx_data = transmission_data[j,9]
                break
                
        if (link_sender,link_dest) in traffic:
            t = traffic[(link_sender,link_dest)][task_name]
            t.append(tx_data/link_dur);
        else:
            t = traffic[(link_dest,link_sender)][task_name]
            t.append(tx_data/link_dur);
            
    
    return traffic