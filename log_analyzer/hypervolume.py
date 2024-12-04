import itertools

import matplotlib.pyplot as plt
from pygmo.core import non_dominated_front_2d, hv2d, hypervolume

from log_reader import Log

def pareto(log: Log):
    fitness_list = [fitness for fitness in set(individual.fitness for individual in log.generations[-1])]
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

# Chuẩn hóa dữ liệu
f1_max = max(fitness[0] for fitness in itertools.chain(pareto1, pareto2, pareto3, pareto4))
f2_max = max(fitness[1] for fitness in itertools.chain(pareto1, pareto2, pareto3, pareto4))

pareto1 = [(fitness[0]/f1_max, fitness[1]/f2_max) for fitness in pareto1]
pareto2 = [(fitness[0]/f1_max, fitness[1]/f2_max) for fitness in pareto2]
pareto3 = [(fitness[0]/f1_max, fitness[1]/f2_max) for fitness in pareto3]
pareto4 = [(fitness[0]/f1_max, fitness[1]/f2_max) for fitness in pareto4]

hv = hypervolume(pareto1)
print(hv.compute((1, 1), hv2d()))

hv = hypervolume(pareto2)
print(hv.compute((1, 1), hv2d()))

hv = hypervolume(pareto3)
print(hv.compute((1, 1), hv2d()))

hv = hypervolume(pareto4)
print(hv.compute((1, 1), hv2d()))

fig, ax = plt.subplots()
ax.scatter(*zip(*pareto1), s=15, label='100 cá thể')
ax.scatter(*zip(*pareto2), s=15, label='150 cá thể')
ax.scatter(*zip(*pareto3), s=15, label='100 cá thể + LS')
ax.scatter(*zip(*pareto4), s=15, label='150 cá thể + LS')

ax.scatter(1, 1, label='ref', c='black')

plt.show()
