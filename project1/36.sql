SELECT t.name AS name 
FROM Trainer t, CatchedPokemon cp 
WHERE cp.pid IN (SELECT after_id FROM Evolution WHERE after_id NOT IN (SELECT e2.before_id FROM Evolution e2)) AND t.id = cp.owner_id
ORDER BY name;