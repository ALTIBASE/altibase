CREATE OR REPLACE FUNCTION addTrcLevel
(p2 IN VARCHAR(8), p1 IN BIGINT ) 
RETURN VARCHAR(64)
AS 
v_rate VARCHAR(64); 
BEGIN 
SELECT 'old->' || powlevel  || ' new->' || powlevel + power(2, (p1 - 1))
INTO v_rate 
FROM ( select powlevel from v$tracelog where module_name = p2 and trclevel = 99);
RETURN v_rate; 
END; 
/ 

CREATE OR REPLACE FUNCTION delTrcLevel
(p2 IN VARCHAR(8), p1 IN BIGINT ) 
RETURN VARCHAR(64)
AS 
v_rate VARCHAR(64); 
BEGIN 
SELECT 'old->' || powlevel  || ' new->' || powlevel - power(2, (p1 - 1))
INTO v_rate 
FROM ( select powlevel from v$tracelog where module_name = p2 and trclevel = 99);
RETURN v_rate; 
END; 
/ 

