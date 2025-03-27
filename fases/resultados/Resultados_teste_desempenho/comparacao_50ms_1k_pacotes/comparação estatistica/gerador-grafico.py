import pandas as pd
import matplotlib.pyplot as plt
import os

# Defina os caminhos dos arquivos CSV
quic_csv = 'latencias_50msQuic.csv'
tcp_csv = 'latencias_50msTCP.csv'
tls_csv = 'latencias_50msTLS.csv'

# Leia os dados dos arquivos CSV
quic_data = pd.read_csv(quic_csv)
tcp_data = pd.read_csv(tcp_csv)
tls_data = pd.read_csv(tls_csv)

# Função para calcular estatísticas básicas
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

# Calcule as estatísticas para cada protocolo
quic_stats = calculate_statistics(quic_data['Latência (ms)'])
tcp_stats = calculate_statistics(tcp_data['Latência (ms)'])
tls_stats = calculate_statistics(tls_data['Latência (ms)'])

# Crie a pasta 'graficos' se ela não existir
if not os.path.exists('graficos'):
    os.makedirs('graficos')

# Função para plotar e salvar gráficos comparativos
def plot_comparison(stats_quic, stats_tcp, stats_tls, stat_name):
    labels = ['QUIC', 'TCP', 'TLS']
    values = [stats_quic[stat_name], stats_tcp[stat_name], stats_tls[stat_name]]
    
    plt.figure(figsize=(10, 6))
    plt.bar(labels, values, color=['blue', 'green', 'red'])
    plt.title(f'Comparison of {stat_name}')
    plt.ylabel(stat_name)
    
    # Salve o gráfico em um arquivo dentro da pasta 'graficos'
    plt.savefig(f'graficos/comparacao_{stat_name}.png')
    plt.close()

# Gráficos comparativos para cada estatística
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
