SELECT p.name
FROM Pokemon p
WHERE p.id NOT IN (SELECT cp.pid FROM CatchedPokemon cp)
ORDER BY p.name;