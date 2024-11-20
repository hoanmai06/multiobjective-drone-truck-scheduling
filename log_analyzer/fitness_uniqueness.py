from log_reader import Log

log = Log('../log/log.txt')

for i, generation in enumerate(log.generations):
    fitness_list = []

    for individual in generation:
        fitness_list.append(individual.fitness)

    print('Generation {}'.format(i))
    print('Số cá thể: {}'.format(len(fitness_list)))
    print('Số fitness phân biệt: {}'.format(len(set(fitness_list))))
