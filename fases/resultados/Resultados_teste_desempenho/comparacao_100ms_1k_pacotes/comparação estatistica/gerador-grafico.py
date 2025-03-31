import pandas as pd
import matplotlib.pyplot as plt
import os
from matplotlib.ticker import FuncFormatter, MaxNLocator

quic_csv = 'latencias_100msQuic.csv'
tcp_csv = 'latencias_100msTCP.csv'
tls_csv = 'latencias_100msTLS.csv'

quic_data = pd.read_csv(quic_csv)
tcp_data = pd.read_csv(tcp_csv)
tls_data = pd.read_csv(tls_csv)

def calculate_statistics(data):
    return {
        'mean': data.mean(),
        'median': data.median(),
        'std': data.std(),
        'percentile_95': data.quantile(0.95),
        'percentile_99': data.quantile(0.99),
        'max': data.max(),
        'min': data.min(),
        'variance': data.var(),
        'skewness': data.skew(),
        'kurtosis': data.kurtosis()
    }

def scientific_formatter(x, pos):
    if x == 0:
        return "0"
    elif x == int(x):
        return f'{int(x)}'
    else:
        return f"${x:.2e}".replace('e', ' \\times 10^{') + "}$"

quic_stats = calculate_statistics(quic_data['Latência (ms)'])
tcp_stats = calculate_statistics(tcp_data['Latência (ms)'])
tls_stats = calculate_statistics(tls_data['Latência (ms)'])


if not os.path.exists('graficos'):
    os.makedirs('graficos')

def plot_comparison(stats_quic, stats_tcp, stats_tls, stat_name):
    labels = ['QUIC', 'TCP', 'TLS']
    values = [stats_quic[stat_name], stats_tcp[stat_name], stats_tls[stat_name]]
    
    max_value = max(stats_quic[stat_name], stats_tcp[stat_name], stats_tls[stat_name])
    min_value = min(stats_quic[stat_name], stats_tcp[stat_name], stats_tls[stat_name])
    
    plt.figure(figsize=(10, 6))
    bars = plt.bar(labels, values, color=['#808080', '#A9A9A9', '#696969'])
    plt.title(f'Comparison of {stat_name}')
    plt.ylabel(stat_name)
    
    plt.ylim(min_value * 0.9, max_value * 1.1)
    
    plt.gca().yaxis.set_major_formatter(FuncFormatter(scientific_formatter))
    plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True, prune='lower'))
    
    for bar in bars:
        yval = bar.get_height()
        plt.text(bar.get_x() + bar.get_width() / 2, yval + 0.00001, 
                 f"${yval:.2e}".replace('e', ' \\times 10^{') + "}$", ha='center', va='bottom', fontsize=10, color='black')

    plt.savefig(f'graficos/comparacao_{stat_name}.png', dpi=300)
    plt.close()

plot_comparison(quic_stats, tcp_stats, tls_stats, 'mean')
plot_comparison(quic_stats, tcp_stats, tls_stats, 'median')
plot_comparison(quic_stats, tcp_stats, tls_stats, 'std')
plot_comparison(quic_stats, tcp_stats, tls_stats, 'percentile_95')
plot_comparison(quic_stats, tcp_stats, tls_stats, 'percentile_99')
plot_comparison(quic_stats, tcp_stats, tls_stats, 'max')
plot_comparison(quic_stats, tcp_stats, tls_stats, 'min')
plot_comparison(quic_stats, tcp_stats, tls_stats, 'variance')
plot_comparison(quic_stats, tcp_stats, tls_stats, 'skewness')
plot_comparison(quic_stats, tcp_stats, tls_stats, 'kurtosis')
