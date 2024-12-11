import itertools

import matplotlib.pyplot as plt
from pygmo.core import non_dominated_front_2d, hv2d, hypervolume

from log_reader import Log

def get_pareto(log: Log):
    fitness_list = [fitness for fitness in set(individual.fitness for individual in log.generations[-1])]
    pareto_indices = non_dominated_front_2d(fitness_list)
    return [fitness_list[index] for index in pareto_indices]

log_files = [
    '../log/50.10.1.n_retry_10.txt',
    '../log/50.10.1_LS_40.txt',
    '../log/50.10.1_LS_30.txt',
    '../log/50.10.1_LS_20.txt',
    '../log/50.10.1_LS_10.txt',
    '../log/50.10.1_LS_1.txt',
]

legends = [
    'NSGA-II',
    'T=40',
    'T=30',
    'T=20',
    'T=10',
    'T=1'
]

logs = [Log(log_file) for log_file in log_files]
paretos = [get_pareto(log) for log in logs]

# Chuẩn hóa dữ liệu
f1_max = max(fitness[0] for fitness in itertools.chain(*paretos))
f2_max = max(fitness[1] for fitness in itertools.chain(*paretos))

for i in range(len(paretos)):
    paretos[i]= [(fitness[0]/f1_max, fitness[1]/f2_max) for fitness in paretos[i]]

for pareto in paretos:
    hv = hypervolume(pareto)
    print(hv.compute((1, 1), hv2d()))

fig, ax = plt.subplots()

for i, pareto in enumerate(paretos):
    ax.scatter(*zip(*pareto), s=15, label=legends[i])

ax.scatter(1, 1, label='ref', c='black')
ax.legend()

plt.show()
