SELECT AVG(cp.level)
FROM CatchedPokemon cp
WHERE cp.owner_id IN (SELECT t.id FROM Trainer t WHERE t.hometown = 'Sangnok City')
	AND cp.pid IN (SELECT p.id FROM Pokemon p WHERE p.type = 'Electric');