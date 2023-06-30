SELECT t_name
FROM 
    (SELECT Trainer.name AS t_name, Count(*) AS c_count
    FROM Trainer, CatchedPokemon
    WHERE Trainer.id = CatchedPokemon.owner_id
    GROUP BY Trainer.name
    ) AS CatchedCount
WHERE c_count >= 3
ORDER BY c_count DESC;