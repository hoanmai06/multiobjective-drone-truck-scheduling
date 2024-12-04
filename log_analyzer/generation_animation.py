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
    text.set_text('Generation {}'.format(frame))

    if frame == 30 or frame == 100 or frame == 150:
        plt.xlim([min(fitness[0] for fitness in fitness_of_generation(frame))/1.6, 1.6*max(fitness[0] for fitness in fitness_of_generation(frame))])
        plt.ylim([min(fitness[1] for fitness in fitness_of_generation(frame))/1.6, 1.6*max(fitness[1] for fitness in fitness_of_generation(frame))])

    return scatter, text

if __name__ == '__main__':
    log = Log('../log/log.txt')

    fig, ax = plt.subplots()
    scatter = ax.scatter(*zip(*list_of_all_fitness()), s=15, c='black')
    text = ax.set_xlabel('Generation 0')

    animation = matplotlib.animation.FuncAnimation(fig, func=update, frames=len(log.generations), interval=60, repeat=False)
    # animation.save('200.10.1_1400_2000.mp4')
    plt.show()
