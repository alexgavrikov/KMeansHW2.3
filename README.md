Всего 4 программы - главный сервер, клиент-раб, обычный клиент, программа-тест (считает метрику на тестовых данных).
У каждой из 4 программ своя функция main, свой makefile и свои дополнительные файлы с кодом. Хотя есть общий код, выделенный
в отдельную папку Common.
Каждой программе выделена папка, где хранится её код  и makefile. 
Ещё есть папка out, куда я при тестировании сохранял файлы с результатами работ программ.



Для того, чтобы собрать программу-сервер надо в папке Server запустить:

	make -f ServerMakefile
Остальные программы аналогично.



Сначала запускается сервер, потом клиенты-рабы, и потом только клиент, причём можно несколько клиентов по очереди, то есть
сервер с кластером способны обслужить несоклько запросов на kmeans-job.

Запуск сервера:

	server_main port_for_slaves slaves_num port_for_clients k_means_jobs_num
Например:

	./server_main 1234 2 1235 1

Запуск раба:

	slave_main port_to_bind
Например:

	./slave_main 1234

Запуск клиента:

	client_main number_of_clusters input_file output_file server_port
Например:

	./client_main 10 shad.learn ../out/K=10_slaves_count=2_centroids.txt 1235
	
Запуск теста:

	test_main input_file_with_test_data input_file_with_centroids output_file
Например:

	/test_main shad.test ../out/K=10_slaves_count=2_centroids.txt ../out/K=10_slaves_count=2_RESULT.txt 



В целях оптимизации с осторожностью использовал класс std::stringstream, так как он медленный. Например, на сервере
мы получаем от клиента данные уже упакованными в строку. Поэтому давайте вместо того, что их извлекать и заново упаковывать
вручную поделим и разошлём рабам-клиентам. Серверу же данные нужны только для начальной инциализации центроидов, но
для этого не обязательно данные извлекать из строки.



Формат вывода центроидов в файл:

 centroid_id:0 \t component \t component \t ... \t component
...
 centroid_id:K-1 \t component \t component \t ... \t component
 
Правила общения между клиентом и сервером:
Формат сообщения:

	content_length \t message
Например:

	3 \t OK

Само сообщение представляет из себя последовательность чисел, разделённых разделителями '#' и ','.
'#' перед каждой новой точкой (то есть перед dimensions компонентами, которых в датасете-примере 10).
',' после каждого числа, будь то компонента точки или что-нибудь ещё. Таким образом да, между точками будут рядом
стоять ',' и '#', ну и ладно.



Резульаты отличаются от результатов программы из домашки 2.1 (где просто пмногопоточность без клиент-серверной архитектуры)
и от домашки 2.2. Это потому что InitCentroidsRandomly по-другому реализована (ради оптимизации по памяти).
