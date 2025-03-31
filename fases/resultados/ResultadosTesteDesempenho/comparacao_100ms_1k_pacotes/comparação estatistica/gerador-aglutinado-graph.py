import pandas as pd
import matplotlib.pyplot as plt
import os
import math

# Defina os caminhos dos arquivos CSV
quic_csv = 'latencias_100msQuic.csv'
tcp_csv = 'latencias_100msTCP.csv'
tls_csv = 'latencias_100msTLS.csv'

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
    print("Diretório 'graficos' criado.")
else:
    print("Diretório 'graficos' já existe.")

# Função para plotar e salvar gráficos comparativos
def plot_comparison(stats_quic, stats_tcp, stats_tls, stat_name, ax):
    labels = ['QUIC', 'TCP', 'TLS']
    values = [stats_quic[stat_name], stats_tcp[stat_name], stats_tls[stat_name]]
    
    ax.bar(labels, values, color=['blue', 'green', 'red'])
    
    titles = {
        'mean': 'Média das Latências',
        'median': 'Mediana das Latências',
        'std': 'Desvio Padrão das Latências',
        'percentile_95': 'Percentil 95 das Latências',
        'percentile_99': 'Percentil 99 das Latências',
        'max': 'Latência Máxima',
        'min': 'Latência Mínima',
        'variance': 'Variância das Latências',
        'skewness': 'Assimetria das Latências',
        'kurtosis': 'Curtose das Latências'
    }
    
    ax.set_title(titles[stat_name])
    ax.set_ylabel('Valor')
    ax.set_xlabel('Protocolo')

# Defina os nomes das estatísticas
stat_names = ['mean', 'median', 'std', 'percentile_95', 'percentile_99', 'max', 'min', 'variance', 'skewness', 'kurtosis']

# Calcule o número de linhas e colunas necessário para os subplots
num_stats = len(stat_names)
num_cols = 4
num_rows = math.ceil(num_stats / num_cols)

# Crie uma figura e uma grade de subplots
fig, axs = plt.subplots(num_rows, num_cols, figsize=(20, 4 * num_rows))
fig.suptitle('Comparação de Estatísticas de Latência', fontsize=16)

# Plote cada gráfico no subplot correspondente
for i, stat_name in enumerate(stat_names):
    row = i // num_cols
    col = i % num_cols
    plot_comparison(quic_stats, tcp_stats, tls_stats, stat_name, axs[row, col])

# Remove os subplots não utilizados
for j in range(num_stats, num_rows * num_cols):
    fig.delaxes(axs[j // num_cols, j % num_cols])

# Ajuste o layout
plt.tight_layout(rect=[0, 0, 1, 0.96])

# Salve o gráfico aglutinado
plt.savefig('graficos/comparacao_geral.png')
plt.close()

print('Gráfico aglutinado salvo como graficos/comparacao_geral.png')
