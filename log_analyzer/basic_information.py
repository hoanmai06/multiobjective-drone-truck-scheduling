from pygmo.core import non_dominated_front_2d

from log_reader import Log

log = Log('../log/50.10.1_150_500_LS.txt')

# In kích thước front
fitness_list = [individual.fitness for individual in log.generations[-1]]
fitness_list = [fitness for fitness in set(fitness_list)]
print('Kích thước front: {}'.format(len(non_dominated_front_2d(fitness_list))))

# In min và max của fitness
print('Fitness 1: min: {}, max: {}'.format(min(fitness[0] for fitness in fitness_list), max(fitness[0] for fitness in fitness_list)))
print('Fitness 2: min: {}, max: {}'.format(min(fitness[1] for fitness in fitness_list), max(fitness[1] for fitness in fitness_list)))
