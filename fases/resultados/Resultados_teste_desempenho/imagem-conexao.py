import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

# Data (convertendo os tempos para milissegundos)
protocols = ['QUIC', 'QUIC', 'TCP', 'TCP', 'TLS/TCP', 'TLS/TCP']
intervals = ['50 MS', '100 MS', '50 MS', '100 MS', '50 MS', '100 MS']
times = [0.003014908 * 1000, 0.003137277 * 1000, 0.000327549 * 1000, 0.000051444 * 1000, 0.011757890 * 1000, 0.012989165 * 1000]  # Multiplicado por 1000 para milissegundos

plt.figure(figsize=(10, 6))

bar_width = 0.25  
index = range(len(times))  
colors = ['#808080', '#A9A9A9', '#696969', '#D3D3D3', '#333333', '#B0B0B0']  # Grayscale

# Create the bars
bars = plt.bar(index, times, color=colors)

# Adjust the X-axis labels to show intervals and protocols
plt.xticks(index, [f'{protocols[i]} ({intervals[i]})' for i in range(len(protocols))], rotation=45, ha="right")

# Add title and axis labels
#plt.title('Connection Time to the Broker by Protocol and Interval')
#plt.xlabel('Protocol and Time Interval')
plt.ylabel('Connection Time (milliseconds)')  # Alterado para milissegundos

plt.ylim(0, max(times) * 1.1)  # Adjust the upper limit of the Y-axis


plt.gca().yaxis.get_major_locator().set_params(integer=True)
plt.gca().yaxis.set_major_formatter(plt.FuncFormatter(lambda x, pos: '{:.2f}'.format(x)))  # Formatação com 2 casas decimais

# Display the values above the bars
for bar in bars:
    yval = bar.get_height()
    plt.text(bar.get_x() + bar.get_width() / 2, yval + 0.00001, 
             f"{yval:.2f}", ha='center', va='bottom', fontsize=10, color='black')  # Mostrar as latências com 2 casas decimais

plt.grid(True, axis='y', linestyle='--', alpha=0.7)

plt.tight_layout()
plt.show()
