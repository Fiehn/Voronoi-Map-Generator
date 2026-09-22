# The Abstract
The abstract world consists of Religion, Culture and language. 
These are interconnected systems that define the identity of groups within the world. 
Each system has its own set of attributes and behaviors that influence how they interact with each other and with the world at large.

Therefore they are conencted inside a flecs ECS world.
They also store their changes to persistent arrays of history entries.
They each have a manager that accepts the flecs::world and handles all logic independt of data.

## Religion
Religions are belief systems that provide a framework for understanding the world and one's place in it.

Religions are defined by their Tenets, Divine Structure, Traits, and Practices (behaviour trees).
### Tenets
Tenets are the core beliefs and principles that define a religion.

### Divine Structure
This decides if the religion is monotheistic, polytheistic, pantheistic, humanist and so on.
Then if relevant the relationships between Divine Entities, their domains and hierarchies are defined.
#### Divine Entities
This is gods, spirits, ancestors, primal forces and so on.
They are defined by domains, relationships to other Divine Entities and traits.

### Traits
Traits are characteristics that define the nature of the religion.
This can influence culture and behaviour of POPs.

### Practices
Practices are the rituals, ceremonies, and behaviors that followers of the religion engage in.
Example:
- Worship rituals
- Festivals
- Moral codes
These are implemented as behaviour trees.
Lets say worship ritual, where they worship a sky god they sacrifice an animal. 
This means the pop will need more livestock resources and if GLOBAL::gods_exist is true they will gain some kind of blessing buff.

## Culture
Cultures are the shared customs, traditions, and social behaviors of a group of people.
Cultures are defined by their Social Structure, Traditions, Values, and Practices (behaviour trees).
### Social Structure
Social Structure defines the organization of society, including class systems, family structures, and governance.
### Traditions
Traditions are the customs and practices that are passed down through generations.
Will ??
### Values
Values are the principles and standards that a culture holds important.
These include:
- Individualism vs collectivism
- Attitudes towards authority
- and so on.
### Practices
Practices are the rituals, ceremonies, and behaviors that are characteristic of a culture.
These are implemented as behaviour trees.
Example:
- Coming of age ceremonies
- Marriage rituals
- Funeral practices
- and so on.
These practices can influence social cohesion, resource allocation, and interactions with other cultures.

## Language
Languages are systems of communication that enable people to convey ideas, emotions, and information.
Languages are defined by their Phonetics, Grammar, Vocabulary, sound change rules, and writing systems.
### Phonetics
Phonetics is the study of the sounds used in a language.
We load the IPA phonetic alphabet and define the phonemes used in the language (sounds).
The sounds are stored as a list of refrences (int) to the IPA phonetic alphabet.
### Grammar
Grammar defines the rules and structure of a language, including syntax, morphology, and sentence construction.
This will be a later addition.
### Vocabulary
Vocabulary is the set of words and phrases used in a language.
Words are generated using the phonemes defined in the Phonetics section.
They are stored as a list of Word {string phonetic_text, int meaning_id, PartOfSpeech pos}
where meaning_id refrences a meaning in a global meaning dictionary.
### Sound Change Rules
Sound Change Rules define how sounds in a language evolve over time.
These rules can be applied to the phonemes to simulate language evolution.
Grimms law is an example of a sound change rule. (go creazy here)
### Writing Systems
Writing Systems are the methods used to visually represent a language.
For now we will just define if the language has a writing system or not.
This can be expanded later to include different types of writing systems (alphabetic, logographic, syllabic, etc.) and their characteristics.
## Integration
The Religion, Culture, and Language systems are interconnected and can influence each other in various ways.
This is why it is stored in the same flecs::world.
For example:
- A religion's tenets may influence cultural values and practices.
- Cultural traditions may shape the development of language and vocabulary.
- Language can be used to express religious beliefs and cultural identity.
- Language influences names of Divine Entities, religious texts, and cultural stories.

## Technical Implementation
Each of the systems (Religion, Culture, Language) will have its own manager class that handles the logic and interactions within that system.
These managers will interact with the flecs::world to create, update, and manage entities and their components.
Each system will also have its own set of data structures to store the relevant information, such as arrays for history entries, lists of tenets, traditions, phonemes, etc.
The managers will also handle the application of behaviour trees for practices in Religion and Culture.
The overall architecture will be designed to allow for easy expansion and modification of each system, enabling the addition of new features and behaviors as needed.