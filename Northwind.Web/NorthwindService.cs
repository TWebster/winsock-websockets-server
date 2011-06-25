using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Data.SqlClient;
using System.Data.Common;
using System.ServiceModel;
using System.ServiceModel.Web;
using System.Reflection;
using System.Linq.Expressions;

namespace Northwind
{
	[ServiceBehavior(InstanceContextMode = InstanceContextMode.Single)]
	public class NorthwindService : INorthwindService
	{
		
		public NorthwindService()
		{
			var connectionStrings = System.Configuration.ConfigurationManager.ConnectionStrings;
			this.connectionString = connectionStrings["northwind"].ConnectionString;			
		}
		string connectionString;		

		public object[] GetObjects()
		{
			return this.FnGetDatabaseObjects(typeof(Supplier));
		}
		public Supplier GetSupplierBad(string id, string co, string city, string json)
		{
			if (string.IsNullOrEmpty(json))				
				json = GetSupplier(id, co, city);

			return json.ParseJSON<Supplier>();				
		}

		public string GetSupplier(string id, string co, string city)
		{		
			int ID = 0;
			using (DbConnection dbconnection = new System.Data.SqlClient.SqlConnection(this.connectionString))
			{
				dbconnection.Open();
				var processor = new DbQueryProcessor(dbconnection);
				var suppliers = from s in processor.Execute<Supplier>()
								let idParsed = int.TryParse(id, out ID)
								where s.Country == co.ToUpper() 
								|| s.City == city
								|| (idParsed && s.ID.HasValue && ID == s.ID.Value)
								select s;
				//return processor.Execute< Supplier>().First();
				Supplier supplier = suppliers.FirstOrDefault();
				string json = supplier.JSON_stringify();
				return json;
			}			
		}
		public  Supplier PostSupplierBad( Supplier s)
		{
			return s;
		}

		public Supplier PostSupplier(string json)
		{
			Supplier s = json.ParseJSON<Supplier>();
			return s;
		}

		public string GetSuppliers(string url, string postal)
		{
			using (DbConnection dbconnection = new System.Data.SqlClient.SqlConnection(this.connectionString))
			{
				dbconnection.Open();
				var processor = new DbQueryProcessor(dbconnection);
				var suppliers = from s in processor.Execute<Supplier>()
								where s.HomePage != null
								select s;
				string json = suppliers.ToArray().JSON_stringify();
				return json;
			}
		}

		dynamic FnGetDatabaseObjects(Type elementType)
		{
			using (DbConnection dbconnection = new System.Data.SqlClient.SqlConnection(this.connectionString))
			{
				dbconnection.Open();
				var processor = new DbQueryProcessor(dbconnection);
				return processor.Execute(elementType);
			}
		}


		#region IClientAccessPolicy Members
		[System.ServiceModel.OperationBehavior]
		public System.IO.Stream GetClientAccessPolicy()
		{
			const string result = @"<?xml version=""1.0"" encoding=""utf-8""?>
<access-policy>
    <cross-domain-access>
        <policy>
            <allow-from http-request-headers=""*"">
                <domain uri=""*""/>
            </allow-from>
            <grant-to>
                <resource path=""/"" include-subpaths=""true""/>				
            </grant-to>
        </policy>
    </cross-domain-access>
</access-policy>";
			//<socket-resource port=""4502-4534"" protocol=""tcp"" />
			if (System.ServiceModel.Web.WebOperationContext.Current != null)
				System.ServiceModel.Web.WebOperationContext.Current.OutgoingResponse.ContentType = "application/xml";
			return new System.IO.MemoryStream(Encoding.UTF8.GetBytes(result));
		}
		#endregion
        
		static void Main()
		{
			//Uri baseAddress = new Uri("http://localhost:8999/");
			using (var host = new WebServiceHost(typeof(NorthwindService)))
			{
				host.Open();
				string baseAddress = host.BaseAddresses.FirstOrDefault().AbsoluteUri ?? "not in .config file";
				Console.WriteLine("Service {0} started.\n\tAddress:\n\t{1}", typeof(NorthwindService).FullName, baseAddress);
				Console.ReadLine();
			}
		}


		#region INorthwindService Members


		public string PostString(string s)
		{
			Supplier supplier = new Supplier { ContactName = "supplier", Name = "new supplier", ID = 3333, Products = new List<Product> { new Product { ID = 444, Name = "gold" } } };
			return supplier.JSON_stringify();
		}

		#endregion
	}
}
