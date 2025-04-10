import matplotlib.pyplot as plt

def create_line_plot(file_name, y_label):
    vectors = []
    with open(file_name + '.txt', 'r') as file:
        for line in file:
            line = line.strip()
            if line:
                vector = list(map(int, filter(None, line.split(','))))
                vector = vector[:-1]  # Remove the last value
                vectors.append(vector)
    for vector in vectors:
        plt.plot(vector)
    plt.xlabel('Refined Characters')
    plt.ylabel(y_label)
    plt.title('Time series of the ' + y_label)
    plt.savefig(file_name + '.svg', format='svg')
    plt.close()

def create_edge_to_node_ratio_plot(node_file, edge_file, output_file_name):
    # Read node count data
    node_vectors = []
    with open(node_file + '.txt', 'r') as file:
        for line in file:
            line = line.strip()
            if line:
                vector = list(map(int, filter(None, line.split(','))))
                vector = vector[:-1]  # Remove the last value
                node_vectors.append(vector)

    # Read edge count data
    edge_vectors = []
    with open(edge_file + '.txt', 'r') as file:
        for line in file:
            line = line.strip()
            if line:
                vector = list(map(int, filter(None, line.split(','))))
                vector = vector[:-1]  # Remove the last value
                edge_vectors.append(vector)

    # Compute edge/node ratio
    ratio_vectors = []
    for edge_vector, node_vector in zip(edge_vectors, node_vectors):
        if len(edge_vector) == len(node_vector):
            ratio_vector = [edge / node if node != 0 else 0 for edge, node in zip(edge_vector, node_vector)]
            ratio_vectors.append(ratio_vector)

    for ratio_vector in ratio_vectors:
        plt.plot(ratio_vector)

    plt.xlabel('Refined Characters')
    plt.ylabel('Edge/Node Ratio')
    plt.title('Time series of the Edge/Node Ratio')
    plt.savefig(output_file_name + '.svg', format='svg')
    plt.close()

# Create individual line plots
create_line_plot(file_name='time_series_node_count', y_label='Node count')
create_line_plot(file_name='time_series_edge_count', y_label='Edge count')

# Create the edge to node ratio plot
create_edge_to_node_ratio_plot(
    node_file='time_series_node_count',
    edge_file='time_series_edge_count',
    output_file_name='edge_to_node_ratio'
)
