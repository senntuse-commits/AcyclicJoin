DO $$
DECLARE
    i INTEGER;
    random_val INTEGER;
BEGIN
    -- 生成一个随机值用于加密
    SELECT FLOOR(1 + (RANDOM() * 10000))::INT INTO random_val;

    FOR i IN 1..100 LOOP
        EXECUTE format(
            -- 'SELECT a + enc_int4_encrypt(%L)  FROM test',
            'SELECT ca_address_sk + enc_int4_encrypt(%L)  FROM customer_address',
            random_val
        );
    END LOOP;
END $$;
