from log_reader import Log
import matplotlib.pyplot as plt
import matplotlib.animation

def list_of_all_fitness():
    result = []
    for generation in log.generations:
        for individual in generation:
            result.append(individual.fitness)
    return result

def fitness_of_generation(generation_index):
    result = []
    for individual in log.generations[generation_index]:
        result.append(individual.fitness)
    return result

def update(frame):
    scatter.set_offsets(fitness_of_generation(frame))
    return scatter

log = Log('../log/log.txt')

fig, ax = plt.subplots()
scatter = ax.scatter(*zip(*list_of_all_fitness()))

animation = matplotlib.animation.FuncAnimation(fig, func=update, frames=len(log.generations), interval=30, repeat=False)
plt.show()
