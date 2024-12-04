from pygmo.core import non_dominated_front_2d

from log_reader import Log
import matplotlib.pyplot as plt

def pareto(log: Log):
    fitness_list = [individual.fitness for individual in log.generations[-1]]
    pareto_indices = non_dominated_front_2d(fitness_list)
    return [fitness_list[index] for index in pareto_indices]

log1 = Log('../log/50.10.1_100_1000.txt')
log2 = Log('../log/50.10.1_150_1000.txt')
log3 = Log('../log/50.10.1_100_500_LS.txt')
log4 = Log('../log/50.10.1_150_500_LS.txt')

pareto1 = pareto(log1)
pareto2 = pareto(log2)
pareto3 = pareto(log3)
pareto4 = pareto(log4)

fig, ax = plt.subplots()
scatter1 = ax.scatter(*zip(*pareto1), s=15, c='black', label='100 cá thể')
scatter2 = ax.scatter(*zip(*pareto2), s=15, label='150 cá thể')
scatter3 = ax.scatter(*zip(*pareto3), s=15, label='100 cá thể + LS')
scatter4 = ax.scatter(*zip(*pareto4), s=15, label='150 cá thể + LS')
ax.legend()

plt.show()
