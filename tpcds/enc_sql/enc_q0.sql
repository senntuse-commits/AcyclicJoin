WITH random_value_cte AS (
    SELECT FLOOR(1 + (RANDOM() * 10000))::INT AS random_value
)
SELECT 
    ca_address_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_ca_address_sk, 
    'customer_address_sk' AS source
FROM customer_address
UNION ALL
SELECT 
    cd_demo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_demo_sk, 
    'customer_demographics_sk' AS source
FROM customer_demographics
UNION ALL
SELECT 
    cd_dep_count + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'customer_demographics_count' AS source
FROM customer_demographics
UNION ALL
SELECT 
    cd_purchase_estimate + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'customer_demographics_estimate' AS source
FROM customer_demographics
UNION ALL
SELECT 
    cd_dep_employed_count + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'customer_demographics_employed_count' AS source
FROM customer_demographics
UNION ALL
SELECT 
    cd_dep_college_count + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'customer_demographics_college_count' AS source
FROM customer_demographics
UNION ALL
SELECT 
    d_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_sk' AS source
FROM date_dim
UNION ALL
SELECT 
    d_month_seq+ enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_month_seq' AS source
FROM date_dim
UNION ALL
SELECT 
    d_week_seq + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_week_seq' AS source
FROM date_dim
UNION ALL
SELECT 
    d_quarter_seq+ enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_quarter_seq' AS source
FROM date_dim
UNION ALL
SELECT 
    d_year + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_year' AS source
FROM date_dim
UNION ALL
SELECT 
    d_dow + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_dow' AS source
FROM date_dim
UNION ALL
SELECT 
    d_moy + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_moy' AS source
FROM date_dim
UNION ALL
SELECT 
    d_dom + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_dom' AS source
FROM date_dim
UNION ALL
SELECT 
    d_qoy + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_qoy' AS source
FROM date_dim
UNION ALL
SELECT 
    d_fy_year + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_fy_year' AS source
FROM date_dim
UNION ALL
SELECT 
    d_fy_quarter_seq + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_fy_quarter_seq' AS source
FROM date_dim
UNION ALL
SELECT 
    d_fy_week_seq + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_fy_week_seq' AS source
FROM date_dim
UNION ALL
SELECT 
    d_first_dom + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_first_dom' AS source
FROM date_dim
UNION ALL
SELECT 
    d_last_dom + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_last_dom' AS source
FROM date_dim
UNION ALL
SELECT 
    d_same_day_ly + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_same_day_ly' AS source
FROM date_dim
UNION ALL
SELECT 
    d_same_day_lq + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'date_dim_same_day_lq' AS source
FROM date_dim
UNION ALL
SELECT 
    w_warehouse_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'warehous_w_warehouse_sk' AS source
FROM warehouse
UNION ALL
SELECT 
    w_warehouse_sq_ft + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'warehous_w_warehouse_sq_ft' AS source
FROM warehouse
UNION ALL
SELECT 
    sm_ship_mode_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'ship_mode_sm_ship_mode_sk' AS source
FROM ship_mode
UNION ALL
SELECT 
    t_time_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'time_dim_t_time_sk' AS source
FROM time_dim
UNION ALL
SELECT 
    t_time + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'time_dim_t_time' AS source
FROM time_dim
UNION ALL
SELECT 
    t_hour + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'time_dim_t_hour' AS source
FROM time_dim
UNION ALL
SELECT 
    t_minute + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'time_dim_t_minute' AS source
FROM time_dim
UNION ALL
SELECT 
    t_second + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'time_dim_t_second' AS source
FROM time_dim
UNION ALL
SELECT 
    r_reason_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'reason_r_reason_sk' AS source
FROM reason
UNION ALL
SELECT 
    ib_income_band_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'income_band_ib_income_band_sk' AS source
FROM income_band
UNION ALL
SELECT 
    ib_lower_bound + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'income_band_ib_lower_bound' AS source
FROM income_band
UNION ALL
SELECT 
    ib_upper_bound + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'income_band_ib_upper_bound' AS source
FROM income_band
UNION ALL
SELECT 
    i_item_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'item_i_item_sk' AS source
FROM item
UNION ALL
SELECT 
    i_brand_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'item_i_brand_id' AS source
FROM item
UNION ALL
SELECT 
    i_class_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'item_i_class_id' AS source
FROM item
UNION ALL
SELECT 
    i_category_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'item_i_category_id' AS source
FROM item
UNION ALL
SELECT 
    i_manufact_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'item_i_manufact_id' AS source
FROM item
UNION ALL
SELECT 
    i_manager_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'item_i_manager_id' AS source
FROM item
UNION ALL
SELECT 
    s_store_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'store_s_store_sk' AS source
FROM store
UNION ALL
SELECT 
    s_closed_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'store_s_closed_date_sk' AS source
FROM store
UNION ALL
SELECT 
    s_number_employees + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'store_s_number_employees' AS source
FROM store
UNION ALL
SELECT 
    s_floor_space + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'store_s_floor_space' AS source
FROM store
UNION ALL
SELECT 
    s_market_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'store_s_market_id' AS source
FROM store
UNION ALL
SELECT 
    s_division_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'store_s_division_id' AS source
FROM store
UNION ALL
SELECT 
    s_company_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_cd_dep_count, 
    'store_s_company_id' AS source
FROM store
UNION ALL
SELECT 
    cc_call_center_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'call_center_cc_call_center_sk' AS source
FROM call_center
UNION ALL
SELECT 
    cc_closed_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'call_center_cc_closed_date_sk' AS source
FROM call_center
UNION ALL
SELECT 
    cc_open_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'call_center_cc_open_date_sk' AS source
FROM call_center
UNION ALL
SELECT 
    cc_employees + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'call_center_cc_employees' AS source
FROM call_center
UNION ALL
SELECT 
    cc_sq_ft + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'call_center_cc_sq_ft' AS source
FROM call_center
UNION ALL
SELECT 
    cc_mkt_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'call_center_cc_mkt_id' AS source
FROM call_center
UNION ALL
SELECT 
    cc_division + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'call_center_cc_division' AS source
FROM call_center
UNION ALL
SELECT 
    cc_company + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'call_center_cc_company' AS source
FROM call_center
UNION ALL
SELECT 
    c_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_customer_sk' AS source
FROM customer
UNION ALL
SELECT 
    c_current_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_current_cdemo_sk' AS source
FROM customer
UNION ALL
SELECT 
    c_current_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_current_hdemo_sk' AS source
FROM customer
UNION ALL
SELECT 
    c_current_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_current_addr_sk' AS source
FROM customer
UNION ALL
SELECT 
    c_first_shipto_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_first_shipto_date_sk' AS source
FROM customer
UNION ALL
SELECT 
    c_first_sales_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_first_sales_date_sk' AS source
FROM customer
UNION ALL
SELECT 
    c_birth_day + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_birth_day' AS source
FROM customer
UNION ALL
SELECT 
    c_birth_month + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_birth_month' AS source
FROM customer
UNION ALL
SELECT 
    c_birth_year + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_birth_year' AS source
FROM customer
UNION ALL
SELECT 
    c_last_review_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'customer_c_last_review_date_sk' AS source
FROM customer
UNION ALL
SELECT 
    web_site_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_site_web_site_sk' AS source
FROM web_site
UNION ALL
SELECT 
    web_open_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_site_web_open_date_sk' AS source
FROM web_site
UNION ALL
SELECT 
    web_close_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_site_web_close_date_sk' AS source
FROM web_site
UNION ALL
SELECT 
    web_mkt_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_site_web_mkt_id' AS source
FROM web_site
UNION ALL
SELECT 
    web_company_id + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_site_web_company_id' AS source
FROM web_site
UNION ALL
SELECT 
    sr_returned_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_returned_date_sk' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_return_time_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_return_time_sk' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_item_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_item_sk' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_customer_sk' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_cdemo_sk' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_hdemo_sk' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_addr_sk' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_store_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_store_sk' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_reason_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_reason_sk' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_ticket_number + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_ticket_number' AS source
FROM store_returns
UNION ALL
SELECT 
    sr_return_quantity + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_returns_sr_return_quantity' AS source
FROM store_returns
UNION ALL
SELECT 
    hd_demo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'household_demographics_hd_demo_sk' AS source
FROM household_demographics
UNION ALL
SELECT 
    hd_income_band_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'household_demographics_hd_income_band_sk' AS source
FROM household_demographics
UNION ALL
SELECT 
    hd_dep_count + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'household_demographics_hd_dep_count' AS source
FROM household_demographics
UNION ALL
SELECT 
    hd_vehicle_count + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'household_demographics_hd_vehicle_count' AS source
FROM household_demographics
UNION ALL
SELECT 
    wp_web_page_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_page_wp_web_page_sk' AS source
FROM web_page
UNION ALL
SELECT 
    wp_creation_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_page_wp_creation_date_sk' AS source
FROM web_page
UNION ALL
SELECT 
    wp_access_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_page_wp_access_date_sk' AS source
FROM web_page
UNION ALL
SELECT 
    wp_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_page_wp_customer_sk' AS source
FROM web_page
UNION ALL
SELECT 
    wp_char_count + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_page_wp_char_count' AS source
FROM web_page
UNION ALL
SELECT 
    wp_link_count + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_page_wp_link_count' AS source
FROM web_page
UNION ALL
SELECT 
    wp_image_count + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_page_wp_image_count' AS source
FROM web_page
UNION ALL
SELECT 
    wp_max_ad_count + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_page_wp_max_ad_count' AS source
FROM web_page
UNION ALL
SELECT 
    p_promo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'promotion_p_promo_sk' AS source
FROM promotion
UNION ALL
SELECT 
    p_start_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'promotion_p_start_date_sk' AS source
FROM promotion
UNION ALL
SELECT 
    p_end_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'promotion_p_end_date_sk' AS source
FROM promotion
UNION ALL
SELECT 
    p_item_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'promotion_p_item_sk' AS source
FROM promotion
UNION ALL
SELECT 
    p_response_target + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'promotion_p_response_target' AS source
FROM promotion
UNION ALL
SELECT 
    cp_catalog_page_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_page_cp_catalog_page_sk' AS source
FROM catalog_page
UNION ALL
SELECT 
    cp_start_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_page_cp_start_date_sk' AS source
FROM catalog_page
UNION ALL
SELECT 
    cp_end_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_page_cp_end_date_sk' AS source
FROM catalog_page
UNION ALL
SELECT 
    cp_catalog_number + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_page_cp_catalog_number' AS source
FROM catalog_page
UNION ALL
SELECT 
    cp_catalog_page_number + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_page_cp_catalog_page_number' AS source
FROM catalog_page
UNION ALL
SELECT 
    inv_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'inventory_inv_date_sk' AS source
FROM inventory
UNION ALL
SELECT 
    inv_item_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'inventory_inv_item_sk' AS source
FROM inventory
UNION ALL
SELECT 
    inv_warehouse_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'inventory_inv_warehouse_sk' AS source
FROM inventory
UNION ALL
SELECT 
    inv_quantity_on_hand + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'inventory_inv_quantity_on_hand' AS source
FROM inventory
UNION ALL
SELECT 
    cr_returned_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_returned_date_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_returned_time_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_returned_time_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_item_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_item_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_refunded_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_refunded_customer_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_refunded_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_refunded_cdemo_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_refunded_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_refunded_hdemo_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_refunded_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_refunded_addr_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_returning_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_returning_customer_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_returning_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_returning_cdemo_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_returning_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_returning_hdemo_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_returning_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_returning_addr_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_call_center_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_call_center_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_catalog_page_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_catalog_page_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_ship_mode_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_ship_mode_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_warehouse_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_warehouse_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_reason_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_reason_sk' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_order_number + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_order_number' AS source
FROM catalog_returns
UNION ALL
SELECT 
    cr_return_quantity  + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_returns_cr_return_quantity ' AS source
FROM catalog_returns
UNION ALL
SELECT 
    wr_returned_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_returned_date_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_returned_time_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_returned_time_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_item_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_item_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_refunded_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_refunded_customer_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_refunded_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_refunded_cdemo_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_refunded_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_refunded_hdemo_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_refunded_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_refunded_addr_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_returning_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_returning_customer_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_returning_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_returning_cdemo_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_returning_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_returning_hdemo_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_returning_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_returning_addr_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_web_page_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_web_page_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_reason_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_reason_sk' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_order_number + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_order_number' AS source
FROM web_returns
UNION ALL
SELECT 
    wr_return_quantity  + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_returns_wr_return_quantity ' AS source
FROM web_returns
UNION ALL
SELECT 
    ws_sold_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_sold_date_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_sold_time_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_sold_time_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_ship_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_ship_date_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_item_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_item_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_bill_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_bill_customer_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_bill_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_bill_cdemo_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_bill_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_bill_hdemo_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_bill_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_bill_addr_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_ship_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_ship_customer_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_ship_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_ship_cdemo_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_ship_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_ship_hdemo_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_ship_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_ship_addr_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_web_page_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_web_page_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_web_site_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_web_site_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_ship_mode_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_ship_mode_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_warehouse_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_warehouse_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_promo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_promo_sk' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_order_number + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_order_number' AS source
FROM web_sales
UNION ALL
SELECT 
    ws_quantity + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'web_sales_ws_quantity' AS source
FROM web_sales
UNION ALL
SELECT 
    cs_sold_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_sold_date_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_sold_time_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_sold_time_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_ship_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_ship_date_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_bill_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_bill_customer_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_bill_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_bill_cdemo_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_bill_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_bill_hdemo_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_bill_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_bill_addr_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_ship_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_ship_customer_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_ship_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_ship_cdemo_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_ship_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_ship_hdemo_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_ship_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_ship_addr_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_call_center_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_call_center_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_catalog_page_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_catalog_page_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_ship_mode_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_ship_mode_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_warehouse_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_warehouse_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_item_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_item_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_promo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_promo_sk' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_order_number + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_order_number' AS source
FROM catalog_sales
UNION ALL
SELECT 
    cs_quantity + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'catalog_sales_cs_quantity' AS source
FROM catalog_sales
UNION ALL
SELECT 
    ss_sold_date_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_sold_date_sk' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_sold_time_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_sold_time_sk' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_item_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_item_sk' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_customer_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_customer_sk' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_cdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_cdemo_sk' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_hdemo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_hdemo_sk' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_addr_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_addr_sk' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_store_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_store_sk' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_promo_sk + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_promo_sk' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_ticket_number + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_ticket_number' AS source
FROM store_sales
UNION ALL
SELECT 
    ss_quantity  + enc_int4_encrypt((SELECT random_value FROM random_value_cte)) AS modified_value,
    'store_sales_ss_quantity ' AS source
FROM store_sales;