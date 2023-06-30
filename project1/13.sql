SELECT p.name, p.id
FROM Pokemon p, (
    SELECT cp.pid AS id
    FROM CatchedPokemon cp
    WHERE cp.owner_id IN (SELECT t.id FROM Trainer t WHERE t.hometown = 'Sangnok City')) sp
WHERE p.id = sp.id
ORDER BY p.id;