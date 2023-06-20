import numpy as np
import matplotlib.pyplot as plt

def plot_frequency_distribution(filename):
    # Read array from text file
    array = np.loadtxt(filename)
    print(np.median(array), np.mean(array))
    # Plot frequency distribution
    bin_interval = 20
    bins = np.arange(0, 3000, bin_interval)
    plt.hist(array, bins=bins, range=(0,5000))
    plt.xlabel('Elapsed Clocks')
    plt.ylabel('Occurrences')
    plt.title('Binary Trie Elapsed Clocks for Searches (Bucketsize = 20)')
    plt.show()

# Example usage
filename = 'fourbittrie_clocks.txt'
plot_frequency_distribution(filename)