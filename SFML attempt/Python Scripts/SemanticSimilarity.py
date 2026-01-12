import pandas as pd
import numpy as np
from sentence_transformers import SentenceTransformer, util

# Load a semantic similarity model


# Find the domain keywords by scraping the ReligionData.hpp file
def load_domain_keywords(file_path="ReligionData.hpp"):
    keywords = []
    with open(file_path, 'r') as file:
        lines = file.readlines()
        inside_enum = False
        for line in lines:
            line = line.strip()
            if line.startswith("enum class DeityDomain : uint8_t {"):
                inside_enum = True
                continue
            if inside_enum:
                if line.startswith("};"):
                    break
                if line and not line.startswith("//"):
                    keyword = line.split('=')[0].strip().rstrip(',')
                    if keyword:  # Make sure it's not empty
                        keywords.append(keyword)
    return keywords



def compute_semantic_similarity(text1, text2):
    embeddings1 = model.encode(text1, convert_to_tensor=True)
    embeddings2 = model.encode(text2, convert_to_tensor=True)
    similarity = util.pytorch_cos_sim(embeddings1, embeddings2)
    return similarity.item()

def create_similarity_matrix_csv(keywords, output_file="Assets/SemanticSimilarity/DomainKeywordSimilarities.csv"):
    n = len(keywords)
    print(f"Creating {n}x{n} similarity matrix...")
    
    # Create an n x n similarity matrix
    similarity_matrix = np.zeros((n, n))
    
    for i, keyword1 in enumerate(keywords):
        print(f"Processing keyword {i+1}/{n}: {keyword1}")
        for j, keyword2 in enumerate(keywords):
            if i == j:
                # Self-similarity is always 1.0
                similarity_matrix[i][j] = 1.0
            elif i < j:
                # Compute similarity only once (matrix is symmetric)
                similarity_score = compute_semantic_similarity(keyword1, keyword2)
                similarity_matrix[i][j] = similarity_score
                similarity_matrix[j][i] = similarity_score  # Symmetric
    
    # Create DataFrame with keywords as both row and column headers
    df = pd.DataFrame(similarity_matrix, index=keywords, columns=keywords)
    df.to_csv(output_file)
    print(f"\nSemantic similarity matrix CSV file created: {output_file}")
    print(f"Matrix shape: {n}x{n}")
    print("\nFirst few rows:")
    print(df.head())


def PlotSimilarityHeatmap(csv_file="Assets/SemanticSimilarity/DomainKeywordSimilarities.csv"):
    import seaborn as sns
    import matplotlib.pyplot as plt

    # Read the CSV file with the first column as index (keywords)
    df = pd.read_csv(csv_file, index_col=0)

    plt.figure(figsize=(12, 10))
    sns.heatmap(df, annot=True, fmt=".2f", cmap="YlGnBu")
    plt.title("Semantic Similarity Heatmap of Domain Keywords")
    plt.show()

if __name__ == "__main__":
    model = SentenceTransformer('all-MiniLM-L6-v2')
    domain_keywords = load_domain_keywords()
    print(f"Loaded {len(domain_keywords)} domain keywords: {domain_keywords}")

    create_similarity_matrix_csv(domain_keywords)
    PlotSimilarityHeatmap()



