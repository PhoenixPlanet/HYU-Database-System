SELECT COUNT(*)
FROM (SELECT DISTINCT p.id FROM Pokemon p WHERE p.id in (SELECT pid FROM CatchedPokemon WHERE owner_id in (SELECT id FROM Trainer WHERE hometown = 'Sangnok City'))) AS SangnokPokemon;