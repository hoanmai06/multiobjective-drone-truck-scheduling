class Individual:
    binary_gene: list[bool]
    permutation_gene: list[int]
    fitness: tuple[float]

    def __init__(self):
        self.binary_gene = []
        self.permutation_gene = []
        self.fitness = tuple()

class Generation(list[Individual]):
    pass

class Log:
    generations: list[Generation]

    def __init__(self, file_path: str):
        self.generations = []

        with open(file_path) as file:
            for generation_texts in file.read().split('P\n')[:-1]:
                generation = Generation()
                for individual_texts in generation_texts.split('I\n')[:-1]:
                    individual = Individual()

                    # binary_gene_text, permutation_gene_text, fitness_text = individual_texts.split('\n')[:-1]
                    fitness_text = individual_texts.split('\n')[0]
                    # individual.binary_gene = list(map(lambda x: False if x == '0' else True, binary_gene_text[1:-1].split(',')))
                    # individual.permutation_gene = list(map(int, permutation_gene_text[1:-1].split(',')))
                    individual.fitness = tuple(map(float, fitness_text[1:-1].split(',')))

                    generation.append(individual)

                self.generations.append(generation)

if __name__ == '__main__':
    log = Log('../log/log.txt')
    print('Hello World!')
