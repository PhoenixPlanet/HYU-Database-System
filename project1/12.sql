SELECT DISTINCT name, type
FROM Pokemon
WHERE id in (SELECT pid FROM CatchedPokemon WHERE level >= 30)
ORDER BY name;