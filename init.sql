CREATE TABLE flowers_base (
    id SERIAL PRIMARY KEY,
    trefle_id INTEGER UNIQUE,
    name_ru VARCHAR(255),
    name_latin VARCHAR(255) NOT NULL,
    family VARCHAR(100),
    genus VARCHAR(100),
    image_url TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE flowers_technical (
    flower_id INTEGER PRIMARY KEY,
    temp_min_c REAL,
    temp_max_c REAL,
    ph_min REAL,
    ph_max REAL,
    light_level INTEGER,
    humidity INTEGER,
    toxicity VARCHAR(50),
    user_notes TEXT,
    
    CONSTRAINT fk_flower
        FOREIGN KEY(flower_id) 
        REFERENCES flowers_base(id)
        ON DELETE CASCADE
);