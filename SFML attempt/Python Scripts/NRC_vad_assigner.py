import json
import os

# Get the script directory
script_dir = os.path.dirname(os.path.abspath(__file__))

# Load NRC-VAD Lexicon
nrc_vad_path = os.path.join(script_dir, "NRC-VAD-Lexicon.txt")
nrc_vad = {}

print("Loading NRC-VAD Lexicon...")
with open(nrc_vad_path, 'r', encoding='utf-8') as f:
    for line in f:
        parts = line.strip().split('\t')
        if len(parts) == 4:
            word = parts[0].lower()
            valence = float(parts[1])
            arousal = float(parts[2])
            dominance = float(parts[3])
            nrc_vad[word] = {
                'valence': valence,
                'arousal': arousal,
                'dominance': dominance
            }

print(f"Loaded {len(nrc_vad)} words from NRC-VAD Lexicon")

# Load domains.json
domains_path = os.path.join(script_dir, "..", "Assets", "Religions", "domains.json")
with open(domains_path, 'r', encoding='utf-8') as f:
    domains = json.load(f)

print(f"\nProcessing {len(domains)} domains...")

# Update domains with NRC-VAD values
for domain_name, domain_data in domains.items():
    domain_key = domain_name.lower()
    
    tries = 0
    while domain_key not in nrc_vad and tries < 3:
        # Try simple transformations to find a match
        if domain_key.endswith('ism'):
            domain_key = domain_key[:-3]  # Remove 'ism'
        elif domain_key.endswith('ity'):
            domain_key = domain_key[:-3]  # Remove 'ity'
        elif domain_key.endswith('s'):
            domain_key = domain_key[:-1]  # Remove plural 's'
        tries += 1

    if domain_key in nrc_vad:
        # Get NRC-VAD values and transform from [0,1] to [-1,1]
        vad_data = nrc_vad[domain_key]
        transformed_valence = vad_data['valence'] * 2 - 1
        transformed_arousal = vad_data['arousal'] * 2 - 1
        transformed_dominance = vad_data['dominance'] * 2 - 1
        
        # Update the domain's semantic vector
        domain_data['semantic_vector']['valence'] = round(transformed_valence, 3)
        domain_data['semantic_vector']['arousal'] = round(transformed_arousal, 3)
        domain_data['semantic_vector']['dominance'] = round(transformed_dominance, 3)
        
        #print(f"✓ Updated '{domain_name}': V={transformed_valence:.3f}, A={transformed_arousal:.3f}, D={transformed_dominance:.3f}")
    else:
        print(f"✗ '{domain_name}' not found in NRC-VAD Lexicon")

# Save updated domains.json
with open(domains_path, 'w', encoding='utf-8') as f:
    json.dump(domains, f, indent=4)

print(f"\n✓ Updated domains saved to {domains_path}")
